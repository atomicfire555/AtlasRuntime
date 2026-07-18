// AtlasRuntime v0.5.2 Stage 22
// Continuous player-cell transition monitoring through xNVSE MainGameLoop.
// Stage 22 preserves all verified Stage 21 behavior and adds experimental
// active-camera position and forward-vector context to each frame_hitch record.

#include <Windows.h>
#include <ShlObj.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

template <
    class Key,
    class Value,
    class Hash = std::hash<Key>,
    class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<std::pair<const Key, Value>>>
using UnorderedMap =
    std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>;

#include <common/ITypes.h>
#include <nvse/PluginAPI.h>

// The legacy xNVSE headers expect a max macro while parsing GameObjects.h.
// Keep the compatibility macro tightly scoped to this include.
#ifndef max
#define ATLAS_DEFINED_MAX_MACRO
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#include <nvse/GameObjects.h>
#ifdef ATLAS_DEFINED_MAX_MACRO
#undef max
#undef ATLAS_DEFINED_MAX_MACRO
#endif

namespace {

PluginHandle g_pluginHandle = kPluginHandle_Invalid;
NVSEMessagingInterface* g_messaging = nullptr;
constexpr UInt32 kTrackedMessageTypeCount = 1024;
volatile LONG g_seenMessageTypes[kTrackedMessageTypeCount]{};
volatile LONG g_runtimeInitialized = 0;

// Stage 13 location-monitor state. The current cell is seeded after a
// successful PostLoadGame so loading a save does not create a false transition.
bool g_locationMonitorInitialized = false;
UInt32 g_lastCellFormId = 0;

// Stage 15 transition-interval state. This is deliberately reset with the
// location monitor so the first real transition after a reset has no bogus
// predecessor interval.
bool g_transitionTimerInitialized = false;
LONGLONG g_previousTransitionQpc = 0;

// Stage 18.1 frame-interval state. A frame_hitch record is emitted only when
// timing crosses from below the threshold to at-or-above the threshold.
bool g_frameTimerInitialized = false;
bool g_frameHitchActive = false;
LONGLONG g_previousFrameQpc = 0;
constexpr double kFrameHitchThresholdMilliseconds = 25.0;

// Stage 20 player-velocity state.
bool g_previousPlayerPositionValid = false;
float g_previousPlayerPositionX = 0.0f;
float g_previousPlayerPositionY = 0.0f;
float g_previousPlayerPositionZ = 0.0f;

// Stage 21 player-acceleration state.
bool g_previousVelocityValid = false;
double g_previousVelocityX = 0.0;
double g_previousVelocityY = 0.0;
double g_previousVelocityZ = 0.0;


// Fallout: New Vegas 1.4.0.525 player singleton pointer slot.
// This avoids linking against the SDK's external player singleton symbol.
constexpr uintptr_t kPlayerSingletonAddress = 0x011DEA3C;

// xNVSE 1.4.0.525 SDK InterfaceManager singleton pointer slot. Stage 22
// uses the SDK-declared InterfaceManager -> SceneGraph -> NiCamera path.
constexpr uintptr_t kInterfaceManagerSingletonAddress = 0x011D8A80;

struct CameraTelemetry {
  bool available = false;
  float positionX = 0.0f;
  float positionY = 0.0f;
  float positionZ = 0.0f;
  float forwardX = 0.0f;
  float forwardY = 0.0f;
  float forwardZ = 0.0f;
};

bool IsReadableMemoryRange(const void* address, size_t size);
bool TryCaptureCameraTelemetry(CameraTelemetry& telemetry);
bool WriteMessageTypeRecord(UInt32 messageType);
bool WriteLifecycleRecord(const char* eventType, UInt32 messageType,
                          const char* detail);
bool WriteCellFormIdRecord(UInt32 messageType, UInt32 formId);
bool WriteCellNameRecord(UInt32 messageType, UInt32 formId,
                         const char* cellName);
bool WriteCellTypeRecord(UInt32 messageType, UInt32 formId,
                         bool isInterior);
bool WriteWorldspaceRecord(UInt32 messageType, UInt32 cellFormId,
                           TESWorldSpace* worldSpace);
bool WriteFrameHitchRecord(UInt32 messageType, LONGLONG qpcTicks,
                           LONGLONG qpcFrequency,
                           LONGLONG frameDeltaTicks,
                           double frameDeltaMilliseconds,
                           double frameDeltaSeconds,
                           UInt32 currentCellFormId,
                           const char* currentCellName, bool isInterior,
                           TESWorldSpace* worldSpace,
                           bool hasGridCoordinates, SInt32 gridX,
                           SInt32 gridY, float playerPositionX,
                           float playerPositionY, float playerPositionZ,
                           bool hasVelocity, double velocityX,
                           double velocityY, double velocityZ,
                           double playerSpeed, bool hasAcceleration,
                           double accelerationX, double accelerationY,
                           double accelerationZ,
                           double accelerationMagnitude,
                           bool hasCameraData, float cameraPositionX,
                           float cameraPositionY, float cameraPositionZ,
                           float cameraForwardX, float cameraForwardY,
                           float cameraForwardZ);
bool WriteCellChangedRecord(UInt32 messageType, UInt32 previousCellFormId,
                            UInt32 currentCellFormId,
                            const char* currentCellName, bool isInterior,
                            TESWorldSpace* worldSpace,
                            bool hasGridCoordinates, SInt32 gridX,
                            SInt32 gridY, float playerPositionX,
                            float playerPositionY, float playerPositionZ);
void ResetLocationMonitor();
void ResetFrameTimer();
void SeedLocationMonitor(TESObjectCELL* parentCell);
void PollFrameTiming(UInt32 messageType);
void PollPlayerCell(UInt32 messageType);
size_t EscapeJsonString(const char* source, char* destination,
                        size_t destinationSize);

bool IsReadableMemoryRange(const void* address, size_t size) {
  if (!address || size == 0) {
    return false;
  }

  const auto begin = reinterpret_cast<uintptr_t>(address);
  if (begin > UINTPTR_MAX - size) {
    return false;
  }
  const uintptr_t end = begin + size;
  uintptr_t cursor = begin;

  while (cursor < end) {
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(reinterpret_cast<const void*>(cursor), &info,
                     sizeof(info)) != sizeof(info) ||
        info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD) != 0 ||
        (info.Protect & PAGE_NOACCESS) != 0) {
      return false;
    }

    const DWORD readable = info.Protect & 0xFF;
    if (readable != PAGE_READONLY && readable != PAGE_READWRITE &&
        readable != PAGE_WRITECOPY && readable != PAGE_EXECUTE_READ &&
        readable != PAGE_EXECUTE_READWRITE &&
        readable != PAGE_EXECUTE_WRITECOPY) {
      return false;
    }

    const uintptr_t regionEnd =
        reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
    if (regionEnd <= cursor) {
      return false;
    }
    cursor = regionEnd;
  }

  return true;
}

bool TryCaptureCameraTelemetry(CameraTelemetry& telemetry) {
  telemetry = CameraTelemetry{};

  // Stage 22 intentionally avoids the SDK's disabled SceneGraph and NiCamera
  // class definitions. Only the documented 32-bit runtime offsets are used:
  // InterfaceManager singleton -> scene graph (+0x04/+0x08) -> camera (+0xDC)
  // -> NiAVObject world transform (+0x64).
  constexpr size_t kPrimarySceneGraphOffset = 0x04;
  constexpr size_t kSecondarySceneGraphOffset = 0x08;
  constexpr size_t kSceneGraphCameraOffset = 0xDC;
  constexpr size_t kCameraWorldTransformOffset = 0x64;
  constexpr size_t kRotationByteSize = 9 * sizeof(float);
  constexpr size_t kTranslationOffset = kRotationByteSize;
  constexpr size_t kWorldTransformByteSize =
      kRotationByteSize + (3 * sizeof(float));

  const auto managerSlot = reinterpret_cast<const void* const*>(
      kInterfaceManagerSingletonAddress);
  if (!IsReadableMemoryRange(managerSlot, sizeof(*managerSlot))) {
    return false;
  }

  const void* const manager = *managerSlot;
  if (!IsReadableMemoryRange(manager, kSecondarySceneGraphOffset +
                                         sizeof(void*))) {
    return false;
  }

  const auto managerBytes = static_cast<const UInt8*>(manager);
  const void* sceneGraph = nullptr;
  std::memcpy(&sceneGraph, managerBytes + kPrimarySceneGraphOffset,
              sizeof(sceneGraph));
  if (!IsReadableMemoryRange(sceneGraph,
                             kSceneGraphCameraOffset + sizeof(void*))) {
    std::memcpy(&sceneGraph, managerBytes + kSecondarySceneGraphOffset,
                sizeof(sceneGraph));
  }
  if (!IsReadableMemoryRange(sceneGraph,
                             kSceneGraphCameraOffset + sizeof(void*))) {
    return false;
  }

  const auto sceneGraphBytes = static_cast<const UInt8*>(sceneGraph);
  const void* camera = nullptr;
  std::memcpy(&camera, sceneGraphBytes + kSceneGraphCameraOffset,
              sizeof(camera));
  if (!IsReadableMemoryRange(camera, kCameraWorldTransformOffset +
                                         kWorldTransformByteSize)) {
    return false;
  }

  float rotation[9]{};
  float translation[3]{};
  const auto cameraBytes = static_cast<const UInt8*>(camera);
  const UInt8* const worldTransform =
      cameraBytes + kCameraWorldTransformOffset;
  std::memcpy(rotation, worldTransform, sizeof(rotation));
  std::memcpy(translation, worldTransform + kTranslationOffset,
              sizeof(translation));

  telemetry.positionX = translation[0];
  telemetry.positionY = translation[1];
  telemetry.positionZ = translation[2];

  // Gamebryo's camera forward basis is local +Y. Read the second column of
  // the world rotation matrix and normalize it defensively.
  const double rawForwardX = rotation[1];
  const double rawForwardY = rotation[4];
  const double rawForwardZ = rotation[7];
  const double length = std::sqrt((rawForwardX * rawForwardX) +
                                  (rawForwardY * rawForwardY) +
                                  (rawForwardZ * rawForwardZ));
  if (!std::isfinite(length) || length <= 0.000001) {
    return false;
  }

  telemetry.forwardX = static_cast<float>(rawForwardX / length);
  telemetry.forwardY = static_cast<float>(rawForwardY / length);
  telemetry.forwardZ = static_cast<float>(rawForwardZ / length);

  if (!std::isfinite(static_cast<double>(telemetry.positionX)) ||
      !std::isfinite(static_cast<double>(telemetry.positionY)) ||
      !std::isfinite(static_cast<double>(telemetry.positionZ)) ||
      !std::isfinite(static_cast<double>(telemetry.forwardX)) ||
      !std::isfinite(static_cast<double>(telemetry.forwardY)) ||
      !std::isfinite(static_cast<double>(telemetry.forwardZ))) {
    telemetry = CameraTelemetry{};
    return false;
  }

  telemetry.available = true;
  return true;
}

void ResetLocationMonitor() {
  g_locationMonitorInitialized = false;
  g_lastCellFormId = 0;
  g_transitionTimerInitialized = false;
  g_previousTransitionQpc = 0;
}

void ResetFrameTimer() {
  g_frameTimerInitialized = false;
  g_frameHitchActive = false;
  g_previousFrameQpc = 0;
  g_previousPlayerPositionValid = false;
  g_previousPlayerPositionX = 0.0f;
  g_previousPlayerPositionY = 0.0f;
  g_previousPlayerPositionZ = 0.0f;
  g_previousVelocityValid = false;
  g_previousVelocityX = 0.0;
  g_previousVelocityY = 0.0;
  g_previousVelocityZ = 0.0;
}

void PollFrameTiming(UInt32 messageType) {
  if (InterlockedCompareExchange(&g_runtimeInitialized, 0, 0) == 0) {
    return;
  }

  LARGE_INTEGER performanceCounter{};
  LARGE_INTEGER performanceFrequency{};
  if (QueryPerformanceCounter(&performanceCounter) == FALSE ||
      QueryPerformanceFrequency(&performanceFrequency) == FALSE ||
      performanceFrequency.QuadPart <= 0) {
    ResetFrameTimer();
    return;
  }

  const auto playerSlot =
      reinterpret_cast<PlayerCharacter* const*>(kPlayerSingletonAddress);
  PlayerCharacter* const player = playerSlot ? *playerSlot : nullptr;

  if (!g_frameTimerInitialized) {
    g_previousFrameQpc = performanceCounter.QuadPart;
    g_frameTimerInitialized = true;

    if (player) {
      g_previousPlayerPositionX = player->posX;
      g_previousPlayerPositionY = player->posY;
      g_previousPlayerPositionZ = player->posZ;
      g_previousPlayerPositionValid = true;
    } else {
      g_previousPlayerPositionValid = false;
    }
    g_previousVelocityValid = false;
    return;
  }

  if (performanceCounter.QuadPart < g_previousFrameQpc) {
    g_previousFrameQpc = performanceCounter.QuadPart;
    g_previousPlayerPositionValid = false;
    g_previousVelocityValid = false;
    return;
  }

  const LONGLONG frameDeltaTicks =
      performanceCounter.QuadPart - g_previousFrameQpc;
  g_previousFrameQpc = performanceCounter.QuadPart;

  const double frameDeltaSeconds =
      static_cast<double>(frameDeltaTicks) /
      static_cast<double>(performanceFrequency.QuadPart);
  const double frameDeltaMilliseconds = frameDeltaSeconds * 1000.0;

  float playerPositionX = 0.0f;
  float playerPositionY = 0.0f;
  float playerPositionZ = 0.0f;
  bool hasVelocity = false;
  double velocityX = 0.0;
  double velocityY = 0.0;
  double velocityZ = 0.0;
  double playerSpeed = 0.0;
  bool hasAcceleration = false;
  double accelerationX = 0.0;
  double accelerationY = 0.0;
  double accelerationZ = 0.0;
  double accelerationMagnitude = 0.0;

  if (player) {
    playerPositionX = player->posX;
    playerPositionY = player->posY;
    playerPositionZ = player->posZ;

    if (g_previousPlayerPositionValid && frameDeltaSeconds > 0.0) {
      velocityX =
          (static_cast<double>(playerPositionX) -
           static_cast<double>(g_previousPlayerPositionX)) /
          frameDeltaSeconds;
      velocityY =
          (static_cast<double>(playerPositionY) -
           static_cast<double>(g_previousPlayerPositionY)) /
          frameDeltaSeconds;
      velocityZ =
          (static_cast<double>(playerPositionZ) -
           static_cast<double>(g_previousPlayerPositionZ)) /
          frameDeltaSeconds;
      playerSpeed =
          std::sqrt((velocityX * velocityX) +
                    (velocityY * velocityY) +
                    (velocityZ * velocityZ));
      hasVelocity = true;

      if (g_previousVelocityValid) {
        accelerationX =
            (velocityX - g_previousVelocityX) / frameDeltaSeconds;
        accelerationY =
            (velocityY - g_previousVelocityY) / frameDeltaSeconds;
        accelerationZ =
            (velocityZ - g_previousVelocityZ) / frameDeltaSeconds;
        accelerationMagnitude =
            std::sqrt((accelerationX * accelerationX) +
                      (accelerationY * accelerationY) +
                      (accelerationZ * accelerationZ));
        hasAcceleration = true;
      }
    }

    g_previousPlayerPositionX = playerPositionX;
    g_previousPlayerPositionY = playerPositionY;
    g_previousPlayerPositionZ = playerPositionZ;
    g_previousPlayerPositionValid = true;

    if (hasVelocity) {
      g_previousVelocityX = velocityX;
      g_previousVelocityY = velocityY;
      g_previousVelocityZ = velocityZ;
      g_previousVelocityValid = true;
    } else {
      g_previousVelocityValid = false;
    }
  } else {
    g_previousPlayerPositionValid = false;
    g_previousVelocityValid = false;
  }

  const bool isHitch =
      frameDeltaMilliseconds >= kFrameHitchThresholdMilliseconds;

  if (isHitch) {
    if (!g_frameHitchActive) {
      UInt32 currentCellFormId = 0;
      const char* currentCellName = "";
      bool isInterior = false;
      TESWorldSpace* worldSpace = nullptr;
      bool hasGridCoordinates = false;
      SInt32 gridX = 0;
      SInt32 gridY = 0;

      if (player) {
        TESObjectCELL* const parentCell = player->parentCell;
        if (parentCell) {
          currentCellFormId = parentCell->refID;
          currentCellName = parentCell->fullName.name.m_data;
          isInterior = parentCell->IsInterior();
          worldSpace = parentCell->worldSpace;

          hasGridCoordinates = !isInterior && worldSpace != nullptr;
          if (hasGridCoordinates) {
            constexpr double kExteriorCellSize = 4096.0;
            gridX = static_cast<SInt32>(
                std::floor(static_cast<double>(playerPositionX) /
                           kExteriorCellSize));
            gridY = static_cast<SInt32>(
                std::floor(static_cast<double>(playerPositionY) /
                           kExteriorCellSize));
          }
        }
      }

      CameraTelemetry cameraTelemetry{};
      TryCaptureCameraTelemetry(cameraTelemetry);

      WriteFrameHitchRecord(
          messageType, performanceCounter.QuadPart,
          performanceFrequency.QuadPart, frameDeltaTicks,
          frameDeltaMilliseconds, frameDeltaSeconds, currentCellFormId,
          currentCellName, isInterior, worldSpace, hasGridCoordinates,
          gridX, gridY, playerPositionX, playerPositionY, playerPositionZ,
          hasVelocity, velocityX, velocityY, velocityZ, playerSpeed,
          hasAcceleration, accelerationX, accelerationY, accelerationZ,
          accelerationMagnitude, cameraTelemetry.available,
          cameraTelemetry.positionX, cameraTelemetry.positionY,
          cameraTelemetry.positionZ, cameraTelemetry.forwardX,
          cameraTelemetry.forwardY, cameraTelemetry.forwardZ);
      g_frameHitchActive = true;
    }
  } else {
    g_frameHitchActive = false;
  }
}

void SeedLocationMonitor(TESObjectCELL* parentCell) {
  if (!parentCell) {
    ResetLocationMonitor();
    return;
  }

  g_lastCellFormId = parentCell->refID;
  g_locationMonitorInitialized = true;
}

void PollPlayerCell(UInt32 messageType) {
  if (InterlockedCompareExchange(&g_runtimeInitialized, 0, 0) == 0) {
    return;
  }

  const auto playerSlot =
      reinterpret_cast<PlayerCharacter* const*>(kPlayerSingletonAddress);
  PlayerCharacter* const player = playerSlot ? *playerSlot : nullptr;
  if (!player) {
    return;
  }

  TESObjectCELL* const parentCell = player->parentCell;
  if (!parentCell) {
    return;
  }

  const UInt32 currentCellFormId = parentCell->refID;

  if (!g_locationMonitorInitialized) {
    SeedLocationMonitor(parentCell);
    return;
  }

  if (currentCellFormId == g_lastCellFormId) {
    return;
  }

  const UInt32 previousCellFormId = g_lastCellFormId;
  const char* const currentCellName = parentCell->fullName.name.m_data;
  const bool isInterior = parentCell->IsInterior();
  TESWorldSpace* const worldSpace = parentCell->worldSpace;

  // Stage 16 derives exterior world-grid coordinates from the player's
  // verified world position. Fallout: New Vegas exterior cells are 4096 game
  // units wide. std::floor is required so negative coordinates map correctly.
  const bool hasGridCoordinates = !isInterior && worldSpace != nullptr;
  SInt32 gridX = 0;
  SInt32 gridY = 0;
  if (hasGridCoordinates) {
    constexpr double kExteriorCellSize = 4096.0;
    gridX = static_cast<SInt32>(
        std::floor(static_cast<double>(player->posX) / kExteriorCellSize));
    gridY = static_cast<SInt32>(
        std::floor(static_cast<double>(player->posY) / kExteriorCellSize));
  }

  // Stage 17 captures the player's exact engine-space position at the same
  // transition point used by Stage 16. No additional polling or callbacks are
  // introduced; these values are emitted only when the cell actually changes.
  const float playerPositionX = player->posX;
  const float playerPositionY = player->posY;
  const float playerPositionZ = player->posZ;

  WriteCellChangedRecord(messageType, previousCellFormId, currentCellFormId,
                         currentCellName, isInterior, worldSpace,
                         hasGridCoordinates, gridX, gridY, playerPositionX,
                         playerPositionY, playerPositionZ);

  g_lastCellFormId = currentCellFormId;
}

void OnNVSEMessage(NVSEMessagingInterface::Message* message) {
  if (!message) {
    return;
  }

  const UInt32 messageType = message->type;
  if (messageType < kTrackedMessageTypeCount &&
      InterlockedCompareExchange(&g_seenMessageTypes[messageType], 1, 0) == 0) {
    WriteMessageTypeRecord(messageType);
  }

  switch (messageType) {
    case NVSEMessagingInterface::kMessage_PostLoad:
      WriteLifecycleRecord("nvse_post_load", messageType,
                           "xNVSE PostLoad observed");
      break;

    case NVSEMessagingInterface::kMessage_PostPostLoad:
      WriteLifecycleRecord("nvse_post_post_load", messageType,
                           "xNVSE PostPostLoad observed");
      break;

    case NVSEMessagingInterface::kMessage_PreLoadGame:
      WriteLifecycleRecord("game_pre_load", messageType,
                           "Saved-game load is beginning");
      ResetLocationMonitor();
      ResetFrameTimer();
      break;

    case NVSEMessagingInterface::kMessage_PostLoadGame:
      WriteLifecycleRecord(
          "game_post_load", messageType,
          "Saved-game load callback completed; payload not inspected");

      if (InterlockedCompareExchange(&g_runtimeInitialized, 1, 0) == 0) {
        WriteLifecycleRecord(
            "runtime_initialized", messageType,
            "AtlasRuntime initialized once after PostLoadGame");

        const auto playerSlot =
            reinterpret_cast<PlayerCharacter* const*>(
                kPlayerSingletonAddress);
        PlayerCharacter* const player =
            playerSlot ? *playerSlot : nullptr;

        WriteLifecycleRecord(
            player ? "player_pointer_valid" : "player_pointer_null",
            messageType,
            player
                ? "Player singleton pointer is available"
                : "Player singleton pointer is not available");

        if (player) {
          // Stage 8's only object-member read.
          TESObjectCELL* const parentCell = player->parentCell;

          WriteLifecycleRecord(
              parentCell ? "parent_cell_valid" : "parent_cell_null",
              messageType,
              parentCell
                  ? "Player parentCell pointer is available"
                  : "Player parentCell pointer is not available");

          if (parentCell) {
            // Preserve the successful Stage 9 FormID read.
            const UInt32 cellFormId = parentCell->refID;
            WriteCellFormIdRecord(messageType, cellFormId);

            // Stage 10's only new cell-data read. TESObjectCELL contains a
            // TESFullName component at offset 0x18 in this SDK.
            const char* const cellName = parentCell->fullName.name.m_data;
            WriteCellNameRecord(messageType, cellFormId, cellName);

            // Stage 11's only new cell query.
            const bool isInterior = parentCell->IsInterior();
            WriteCellTypeRecord(messageType, cellFormId, isInterior);

            // Stage 12's only new SDK read.
            TESWorldSpace* const worldSpace = parentCell->worldSpace;
            WriteWorldspaceRecord(messageType, cellFormId, worldSpace);

            // Stage 13 seeds the runtime monitor from the verified cell.
            SeedLocationMonitor(parentCell);
          } else {
            ResetLocationMonitor();
          }
        } else {
          ResetLocationMonitor();
        }
      }
      break;

    case NVSEMessagingInterface::kMessage_MainGameLoop:
      PollFrameTiming(messageType);
      PollPlayerCell(messageType);
      break;

    case NVSEMessagingInterface::kMessage_NewGame:
      WriteLifecycleRecord("new_game", messageType,
                           "New-game lifecycle message observed");
      ResetLocationMonitor();
      ResetFrameTimer();
      break;

    case NVSEMessagingInterface::kMessage_ExitToMainMenu:
      WriteLifecycleRecord("exit_to_main_menu", messageType,
                           "Return-to-main-menu message observed");
      ResetLocationMonitor();
      ResetFrameTimer();
      break;

    case NVSEMessagingInterface::kMessage_ExitGame:
      WriteLifecycleRecord("game_exit", messageType,
                           "Game-exit message observed");
      break;

    case NVSEMessagingInterface::kMessage_ExitGame_Console:
      WriteLifecycleRecord("game_exit_console", messageType,
                           "Console game-exit message observed");
      break;

    default:
      break;
  }
}

bool EnsureDirectory(const wchar_t* path) {
  const int result = SHCreateDirectoryExW(nullptr, path, nullptr);
  return result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS ||
         GetLastError() == ERROR_ALREADY_EXISTS;
}

bool GetLogPath(wchar_t (&logPath)[MAX_PATH]) {
  wchar_t documents[MAX_PATH]{};
  if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr,
                             SHGFP_TYPE_CURRENT, documents))) {
    return false;
  }

  wchar_t directory[MAX_PATH]{};
  if (_snwprintf_s(
          directory, _countof(directory), _TRUNCATE,
          L"%s\\My Games\\FalloutNV\\NVSE\\Plugins\\AtlasRuntime",
          documents) < 0 ||
      !EnsureDirectory(directory)) {
    return false;
  }

  return _snwprintf_s(logPath, _countof(logPath), _TRUNCATE,
                      L"%s\\atlas_runtime.jsonl", directory) >= 0;
}

bool WriteWorldspaceRecord(UInt32 messageType, UInt32 cellFormId,
                           TESWorldSpace* worldSpace) {
  wchar_t logPath[MAX_PATH]{};
  if (!GetLogPath(logPath)) {
    return false;
  }

  HANDLE file = CreateFileW(logPath, FILE_APPEND_DATA, FILE_SHARE_READ,
                            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                            nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }

  SYSTEMTIME time{};
  GetSystemTime(&time);

  const bool hasWorldspace = worldSpace != nullptr;
  const UInt32 worldspaceFormId = hasWorldspace ? worldSpace->refID : 0;

  char line[896]{};
  const int length = std::snprintf(
      line, sizeof(line),
      "{\"schema\":\"atlas.runtime.event/1\","
      "\"timestampUtc\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ\","
      "\"eventType\":\"player_worldspace\",\"messageType\":%u,"
      "\"cellFormId\":%u,\"cellFormIdHex\":\"%08X\","
      "\"hasWorldspace\":%s,"
      "\"worldspaceFormId\":%u,\"worldspaceFormIdHex\":\"%08X\"}\r\n",
      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
      time.wSecond, time.wMilliseconds,
      static_cast<unsigned>(messageType),
      static_cast<unsigned>(cellFormId),
      static_cast<unsigned>(cellFormId),
      hasWorldspace ? "true" : "false",
      static_cast<unsigned>(worldspaceFormId),
      static_cast<unsigned>(worldspaceFormId));

  DWORD written = 0;
  const bool success =
      length > 0 && length < static_cast<int>(sizeof(line)) &&
      WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr) &&
      written == static_cast<DWORD>(length);

  FlushFileBuffers(file);
  CloseHandle(file);
  return success;
}

bool WriteFrameHitchRecord(UInt32 messageType, LONGLONG qpcTicks,
                           LONGLONG qpcFrequency,
                           LONGLONG frameDeltaTicks,
                           double frameDeltaMilliseconds,
                           double frameDeltaSeconds,
                           UInt32 currentCellFormId,
                           const char* currentCellName, bool isInterior,
                           TESWorldSpace* worldSpace,
                           bool hasGridCoordinates, SInt32 gridX,
                           SInt32 gridY, float playerPositionX,
                           float playerPositionY, float playerPositionZ,
                           bool hasVelocity, double velocityX,
                           double velocityY, double velocityZ,
                           double playerSpeed, bool hasAcceleration,
                           double accelerationX, double accelerationY,
                           double accelerationZ,
                           double accelerationMagnitude,
                           bool hasCameraData, float cameraPositionX,
                           float cameraPositionY, float cameraPositionZ,
                           float cameraForwardX, float cameraForwardY,
                           float cameraForwardZ) {
  wchar_t logPath[MAX_PATH]{};
  if (!GetLogPath(logPath)) {
    return false;
  }

  HANDLE file = CreateFileW(logPath, FILE_APPEND_DATA, FILE_SHARE_READ,
                            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                            nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }

  SYSTEMTIME time{};
  GetSystemTime(&time);

  char escapedName[1024]{};
  EscapeJsonString(currentCellName ? currentCellName : "", escapedName,
                   sizeof(escapedName));

  const bool hasCurrentCell = currentCellFormId != 0;
  const bool hasWorldspace = worldSpace != nullptr;
  const UInt32 worldspaceFormId = hasWorldspace ? worldSpace->refID : 0;

  char line[4096]{};
  const int length = std::snprintf(
      line, sizeof(line),
      "{\"schema\":\"atlas.runtime.event/1\","
      "\"timestampUtc\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ\","
      "\"eventType\":\"frame_hitch\",\"messageType\":%u,"
      "\"pluginVersion\":52,"
      "\"hitchThresholdMilliseconds\":%.6f,"
      "\"qpcTicks\":%lld,\"qpcFrequency\":%lld,"
      "\"frameDeltaTicks\":%lld,"
      "\"frameDeltaMilliseconds\":%.6f,"
      "\"frameDeltaSeconds\":%.9f,"
      "\"currentCellFormId\":%u,"
      "\"currentCellFormIdHex\":\"%08X\","
      "\"currentCellName\":\"%s\","
      "\"currentCellType\":\"%s\","
      "\"isInterior\":%s,"
      "\"hasWorldspace\":%s,"
      "\"worldspaceFormId\":%u,"
      "\"worldspaceFormIdHex\":\"%08X\","
      "\"hasGridCoordinates\":%s,"
      "\"gridX\":%d,\"gridY\":%d,"
      "\"playerPositionX\":%.6f,"
      "\"playerPositionY\":%.6f,"
      "\"playerPositionZ\":%.6f,"
      "\"hasVelocity\":%s,"
      "\"velocityX\":%.6f,"
      "\"velocityY\":%.6f,"
      "\"velocityZ\":%.6f,"
      "\"playerSpeed\":%.6f,"
      "\"hasAcceleration\":%s,"
      "\"accelerationX\":%.6f,"
      "\"accelerationY\":%.6f,"
      "\"accelerationZ\":%.6f,"
      "\"accelerationMagnitude\":%.6f,"
      "\"hasCameraData\":%s,"
      "\"cameraPositionX\":%.6f,"
      "\"cameraPositionY\":%.6f,"
      "\"cameraPositionZ\":%.6f,"
      "\"cameraForwardX\":%.6f,"
      "\"cameraForwardY\":%.6f,"
      "\"cameraForwardZ\":%.6f}\r\n",
      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
      time.wSecond, time.wMilliseconds,
      static_cast<unsigned>(messageType),
      kFrameHitchThresholdMilliseconds,
      static_cast<long long>(qpcTicks),
      static_cast<long long>(qpcFrequency),
      static_cast<long long>(frameDeltaTicks),
      frameDeltaMilliseconds,
      frameDeltaSeconds,
      static_cast<unsigned>(currentCellFormId),
      static_cast<unsigned>(currentCellFormId),
      escapedName,
      hasCurrentCell ? (isInterior ? "interior" : "exterior") : "unknown",
      isInterior ? "true" : "false",
      hasWorldspace ? "true" : "false",
      static_cast<unsigned>(worldspaceFormId),
      static_cast<unsigned>(worldspaceFormId),
      hasGridCoordinates ? "true" : "false",
      static_cast<int>(gridX),
      static_cast<int>(gridY),
      static_cast<double>(playerPositionX),
      static_cast<double>(playerPositionY),
      static_cast<double>(playerPositionZ),
      hasVelocity ? "true" : "false",
      velocityX,
      velocityY,
      velocityZ,
      playerSpeed,
      hasAcceleration ? "true" : "false",
      accelerationX,
      accelerationY,
      accelerationZ,
      accelerationMagnitude,
      hasCameraData ? "true" : "false",
      static_cast<double>(cameraPositionX),
      static_cast<double>(cameraPositionY),
      static_cast<double>(cameraPositionZ),
      static_cast<double>(cameraForwardX),
      static_cast<double>(cameraForwardY),
      static_cast<double>(cameraForwardZ));

  DWORD written = 0;
  const bool success =
      length > 0 && length < static_cast<int>(sizeof(line)) &&
      WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr) &&
      written == static_cast<DWORD>(length);

  FlushFileBuffers(file);
  CloseHandle(file);
  return success;
}

bool WriteCellChangedRecord(UInt32 messageType, UInt32 previousCellFormId,
                            UInt32 currentCellFormId,
                            const char* currentCellName, bool isInterior,
                            TESWorldSpace* worldSpace,
                            bool hasGridCoordinates, SInt32 gridX,
                            SInt32 gridY, float playerPositionX,
                            float playerPositionY, float playerPositionZ) {
  wchar_t logPath[MAX_PATH]{};
  if (!GetLogPath(logPath)) {
    return false;
  }

  HANDLE file = CreateFileW(logPath, FILE_APPEND_DATA, FILE_SHARE_READ,
                            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                            nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }

  SYSTEMTIME time{};
  GetSystemTime(&time);

  char escapedName[1024]{};
  EscapeJsonString(currentCellName ? currentCellName : "", escapedName,
                   sizeof(escapedName));

  const bool hasWorldspace = worldSpace != nullptr;
  const UInt32 worldspaceFormId = hasWorldspace ? worldSpace->refID : 0;

  LARGE_INTEGER performanceCounter{};
  LARGE_INTEGER performanceFrequency{};
  const bool hasPerformanceCounter =
      QueryPerformanceCounter(&performanceCounter) != FALSE &&
      QueryPerformanceFrequency(&performanceFrequency) != FALSE &&
      performanceFrequency.QuadPart > 0;

  bool hasPreviousCellChange = false;
  LONGLONG transitionDeltaTicks = 0;
  double transitionDeltaMilliseconds = 0.0;
  double transitionDeltaSeconds = 0.0;

  if (hasPerformanceCounter) {
    if (g_transitionTimerInitialized &&
        performanceCounter.QuadPart >= g_previousTransitionQpc) {
      transitionDeltaTicks =
          performanceCounter.QuadPart - g_previousTransitionQpc;
      transitionDeltaSeconds =
          static_cast<double>(transitionDeltaTicks) /
          static_cast<double>(performanceFrequency.QuadPart);
      transitionDeltaMilliseconds = transitionDeltaSeconds * 1000.0;
      hasPreviousCellChange = true;
    }

    g_previousTransitionQpc = performanceCounter.QuadPart;
    g_transitionTimerInitialized = true;
  }

  char line[3200]{};
  const int length = std::snprintf(
      line, sizeof(line),
      "{\"schema\":\"atlas.runtime.event/1\","
      "\"timestampUtc\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ\","
      "\"eventType\":\"player_cell_changed\",\"messageType\":%u,"
      "\"previousCellFormId\":%u,\"previousCellFormIdHex\":\"%08X\","
      "\"currentCellFormId\":%u,\"currentCellFormIdHex\":\"%08X\","
      "\"currentCellName\":\"%s\","
      "\"currentCellType\":\"%s\",\"isInterior\":%s,"
      "\"hasWorldspace\":%s,"
      "\"worldspaceFormId\":%u,\"worldspaceFormIdHex\":\"%08X\","
      "\"hasGridCoordinates\":%s,\"gridX\":%d,\"gridY\":%d,"
      "\"playerPositionX\":%.6f,\"playerPositionY\":%.6f,"
      "\"playerPositionZ\":%.6f,"
      "\"hasPerformanceCounter\":%s,"
      "\"qpcTicks\":%lld,\"qpcFrequency\":%lld,"
      "\"hasPreviousCellChange\":%s,"
      "\"transitionDeltaTicks\":%lld,"
      "\"transitionDeltaMilliseconds\":%.6f,"
      "\"transitionDeltaSeconds\":%.9f}\r\n",
      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
      time.wSecond, time.wMilliseconds,
      static_cast<unsigned>(messageType),
      static_cast<unsigned>(previousCellFormId),
      static_cast<unsigned>(previousCellFormId),
      static_cast<unsigned>(currentCellFormId),
      static_cast<unsigned>(currentCellFormId),
      escapedName,
      isInterior ? "interior" : "exterior",
      isInterior ? "true" : "false",
      hasWorldspace ? "true" : "false",
      static_cast<unsigned>(worldspaceFormId),
      static_cast<unsigned>(worldspaceFormId),
      hasGridCoordinates ? "true" : "false",
      static_cast<int>(gridX),
      static_cast<int>(gridY),
      static_cast<double>(playerPositionX),
      static_cast<double>(playerPositionY),
      static_cast<double>(playerPositionZ),
      hasPerformanceCounter ? "true" : "false",
      hasPerformanceCounter
          ? static_cast<long long>(performanceCounter.QuadPart)
          : 0LL,
      hasPerformanceCounter
          ? static_cast<long long>(performanceFrequency.QuadPart)
          : 0LL,
      hasPreviousCellChange ? "true" : "false",
      static_cast<long long>(transitionDeltaTicks),
      transitionDeltaMilliseconds,
      transitionDeltaSeconds);

  DWORD written = 0;
  const bool success =
      length > 0 && length < static_cast<int>(sizeof(line)) &&
      WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr) &&
      written == static_cast<DWORD>(length);

  FlushFileBuffers(file);
  CloseHandle(file);
  return success;
}

bool WriteCellTypeRecord(UInt32 messageType, UInt32 formId,
                         bool isInterior) {
  wchar_t logPath[MAX_PATH]{};
  if (!GetLogPath(logPath)) {
    return false;
  }

  HANDLE file = CreateFileW(logPath, FILE_APPEND_DATA, FILE_SHARE_READ,
                            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                            nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }

  SYSTEMTIME time{};
  GetSystemTime(&time);

  char line[768]{};
  const int length = std::snprintf(
      line, sizeof(line),
      "{\"schema\":\"atlas.runtime.event/1\","
      "\"timestampUtc\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ\","
      "\"eventType\":\"player_cell_type\",\"messageType\":%u,"
      "\"formId\":%u,\"formIdHex\":\"%08X\","
      "\"isInterior\":%s,\"cellType\":\"%s\"}\r\n",
      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
      time.wSecond, time.wMilliseconds,
      static_cast<unsigned>(messageType),
      static_cast<unsigned>(formId),
      static_cast<unsigned>(formId),
      isInterior ? "true" : "false",
      isInterior ? "interior" : "exterior");

  DWORD written = 0;
  const bool success =
      length > 0 && length < static_cast<int>(sizeof(line)) &&
      WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr) &&
      written == static_cast<DWORD>(length);

  FlushFileBuffers(file);
  CloseHandle(file);
  return success;
}

bool WriteStartupRecord(const char* eventType, const char* detail) {
  wchar_t logPath[MAX_PATH]{};
  if (!GetLogPath(logPath)) {
    return false;
  }

  HANDLE file = CreateFileW(logPath, FILE_APPEND_DATA, FILE_SHARE_READ,
                            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                            nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }

  SYSTEMTIME time{};
  GetSystemTime(&time);

  char line[640]{};
  const int length = std::snprintf(
      line, sizeof(line),
      "{\"schema\":\"atlas.runtime.event/1\","
      "\"timestampUtc\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ\","
      "\"eventType\":\"%s\",\"detail\":\"%s\"}\r\n",
      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
      time.wSecond, time.wMilliseconds, eventType, detail);

  DWORD written = 0;
  const bool success =
      length > 0 && length < static_cast<int>(sizeof(line)) &&
      WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr) &&
      written == static_cast<DWORD>(length);

  FlushFileBuffers(file);
  CloseHandle(file);
  return success;
}

bool WriteMessageTypeRecord(UInt32 messageType) {
  wchar_t logPath[MAX_PATH]{};
  if (!GetLogPath(logPath)) {
    return false;
  }

  HANDLE file = CreateFileW(logPath, FILE_APPEND_DATA, FILE_SHARE_READ,
                            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                            nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }

  SYSTEMTIME time{};
  GetSystemTime(&time);

  char line[384]{};
  const int length = std::snprintf(
      line, sizeof(line),
      "{\"schema\":\"atlas.runtime.event/1\","
      "\"timestampUtc\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ\","
      "\"eventType\":\"nvse_message\",\"messageType\":%u}\r\n",
      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
      time.wSecond, time.wMilliseconds,
      static_cast<unsigned>(messageType));

  DWORD written = 0;
  const bool success =
      length > 0 && length < static_cast<int>(sizeof(line)) &&
      WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr) &&
      written == static_cast<DWORD>(length);

  CloseHandle(file);
  return success;
}

bool WriteLifecycleRecord(const char* eventType, UInt32 messageType,
                          const char* detail) {
  wchar_t logPath[MAX_PATH]{};
  if (!GetLogPath(logPath)) {
    return false;
  }

  HANDLE file = CreateFileW(logPath, FILE_APPEND_DATA, FILE_SHARE_READ,
                            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                            nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }

  SYSTEMTIME time{};
  GetSystemTime(&time);

  char line[640]{};
  const int length = std::snprintf(
      line, sizeof(line),
      "{\"schema\":\"atlas.runtime.event/1\","
      "\"timestampUtc\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ\","
      "\"eventType\":\"%s\",\"messageType\":%u,"
      "\"detail\":\"%s\"}\r\n",
      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
      time.wSecond, time.wMilliseconds, eventType,
      static_cast<unsigned>(messageType), detail);

  DWORD written = 0;
  const bool success =
      length > 0 && length < static_cast<int>(sizeof(line)) &&
      WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr) &&
      written == static_cast<DWORD>(length);

  FlushFileBuffers(file);
  CloseHandle(file);
  return success;
}


bool WriteCellFormIdRecord(UInt32 messageType, UInt32 formId) {
  wchar_t logPath[MAX_PATH]{};
  if (!GetLogPath(logPath)) {
    return false;
  }

  HANDLE file = CreateFileW(logPath, FILE_APPEND_DATA, FILE_SHARE_READ,
                            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                            nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }

  SYSTEMTIME time{};
  GetSystemTime(&time);

  char line[512]{};
  const int length = std::snprintf(
      line, sizeof(line),
      "{\"schema\":\"atlas.runtime.event/1\","
      "\"timestampUtc\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ\","
      "\"eventType\":\"player_cell_form_id\",\"messageType\":%u,"
      "\"formId\":%u,\"formIdHex\":\"%08X\"}\r\n",
      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
      time.wSecond, time.wMilliseconds,
      static_cast<unsigned>(messageType),
      static_cast<unsigned>(formId),
      static_cast<unsigned>(formId));

  DWORD written = 0;
  const bool success =
      length > 0 && length < static_cast<int>(sizeof(line)) &&
      WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr) &&
      written == static_cast<DWORD>(length);

  FlushFileBuffers(file);
  CloseHandle(file);
  return success;
}


size_t EscapeJsonString(const char* source, char* destination,
                        size_t destinationSize) {
  if (!destination || destinationSize == 0) {
    return 0;
  }

  destination[0] = '\0';
  if (!source) {
    return 0;
  }

  size_t written = 0;
  for (const unsigned char* cursor =
           reinterpret_cast<const unsigned char*>(source);
       *cursor != '\0'; ++cursor) {
    const unsigned char value = *cursor;
    const char* escape = nullptr;
    char unicodeEscape[7]{};

    switch (value) {
      case '\"': escape = "\\\""; break;
      case '\\': escape = "\\\\"; break;
      case '\b': escape = "\\b"; break;
      case '\f': escape = "\\f"; break;
      case '\n': escape = "\\n"; break;
      case '\r': escape = "\\r"; break;
      case '\t': escape = "\\t"; break;
      default:
        if (value < 0x20) {
          std::snprintf(unicodeEscape, sizeof(unicodeEscape),
                        "\\u%04X", static_cast<unsigned>(value));
          escape = unicodeEscape;
        }
        break;
    }

    if (escape) {
      const size_t escapeLength = std::strlen(escape);
      if (written + escapeLength >= destinationSize) {
        break;
      }
      std::memcpy(destination + written, escape, escapeLength);
      written += escapeLength;
    } else {
      if (written + 1 >= destinationSize) {
        break;
      }
      destination[written++] = static_cast<char>(value);
    }
  }

  destination[written] = '\0';
  return written;
}

bool WriteCellNameRecord(UInt32 messageType, UInt32 formId,
                         const char* cellName) {
  wchar_t logPath[MAX_PATH]{};
  if (!GetLogPath(logPath)) {
    return false;
  }

  HANDLE file = CreateFileW(logPath, FILE_APPEND_DATA, FILE_SHARE_READ,
                            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                            nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }

  SYSTEMTIME time{};
  GetSystemTime(&time);

  char escapedName[1024]{};
  EscapeJsonString(cellName ? cellName : "", escapedName,
                   sizeof(escapedName));

  char line[1536]{};
  const int length = std::snprintf(
      line, sizeof(line),
      "{\"schema\":\"atlas.runtime.event/1\","
      "\"timestampUtc\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ\","
      "\"eventType\":\"player_cell_name\",\"messageType\":%u,"
      "\"formId\":%u,\"formIdHex\":\"%08X\","
      "\"hasName\":%s,\"name\":\"%s\"}\r\n",
      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
      time.wSecond, time.wMilliseconds,
      static_cast<unsigned>(messageType),
      static_cast<unsigned>(formId),
      static_cast<unsigned>(formId),
      (cellName && cellName[0] != '\0') ? "true" : "false",
      escapedName);

  DWORD written = 0;
  const bool success =
      length > 0 && length < static_cast<int>(sizeof(line)) &&
      WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr) &&
      written == static_cast<DWORD>(length);

  FlushFileBuffers(file);
  CloseHandle(file);
  return success;
}

}  // namespace

extern "C" __declspec(dllexport) bool NVSEPlugin_Query(
    const NVSEInterface* nvse, PluginInfo* info) {
  if (!nvse || !info) {
    return false;
  }

  info->infoVersion = PluginInfo::kInfoVersion;
  info->name = "AtlasRuntime";
  info->version = 52;

  return !nvse->isEditor;
}

extern "C" __declspec(dllexport) bool NVSEPlugin_Load(
    const NVSEInterface* nvse) {
  if (!nvse) {
    return false;
  }

  g_pluginHandle = nvse->GetPluginHandle();
  g_messaging = static_cast<NVSEMessagingInterface*>(
      nvse->QueryInterface(kInterface_Messaging));

  if (g_pluginHandle == kPluginHandle_Invalid || !g_messaging) {
    WriteStartupRecord(
        "plugin_load_failed",
        "AtlasRuntime Stage 22 could not obtain the xNVSE messaging interface");
    return false;
  }

  if (g_messaging->version < 4) {
    WriteStartupRecord(
        "plugin_load_failed",
        "AtlasRuntime Stage 22 requires xNVSE Messaging Interface version 4");
    return false;
  }

  if (!g_messaging->RegisterListener(
          g_pluginHandle, "NVSE", OnNVSEMessage)) {
    WriteStartupRecord(
        "plugin_load_failed",
        "AtlasRuntime Stage 22 could not register the xNVSE listener");
    return false;
  }

  return WriteStartupRecord(
      "plugin_loaded",
      "AtlasRuntime v0.5.2 Stage 22 loaded; edge-triggered 25 ms frame-hitch telemetry with player-location, velocity, acceleration, and experimental camera context enabled");
}
