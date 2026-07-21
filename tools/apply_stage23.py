from pathlib import Path
import sys

source_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
text = source_path.read_text(encoding="utf-8-sig")


def replace_once(old: str, new: str) -> None:
    global text
    if text.count(old) != 1:
        raise RuntimeError(f"Expected exactly one match, found {text.count(old)}: {old[:80]!r}")
    text = text.replace(old, new, 1)

replace_once(
    "// AtlasRuntime v0.5.2 Stage 22\n// Continuous player-cell transition monitoring through xNVSE MainGameLoop.\n// Stage 22 preserves all verified Stage 21 behavior and adds experimental\n// active-camera position and forward-vector context to each frame_hitch record.",
    "// AtlasRuntime v0.5.3 Stage 23\n// Continuous player-cell transition monitoring through xNVSE MainGameLoop.\n// Stage 23 preserves all verified Stage 22 behavior and adds an allocation-free\n// rolling pre-hitch context window with compact summary records.",
)

replace_once("#include <algorithm>\n", "#include <algorithm>\n#include <array>\n")

replace_once(
    "double g_previousVelocityZ = 0.0;\n\n\n// Fallout: New Vegas 1.4.0.525 player singleton pointer slot.",
    """double g_previousVelocityZ = 0.0;

// Stage 23 rolling pre-hitch context. The fixed-size ring performs no heap
// allocation in MainGameLoop and retains approximately two seconds at 60 FPS.
struct FrameContextSample {
  LONGLONG qpcTicks = 0;
  double frameDeltaMilliseconds = 0.0;
  UInt32 cellFormId = 0;
  float playerPositionX = 0.0f;
  float playerPositionY = 0.0f;
  float playerPositionZ = 0.0f;
  bool hasVelocity = false;
  double playerSpeed = 0.0;
  bool hasAcceleration = false;
  double accelerationMagnitude = 0.0;
  bool hasCameraData = false;
  float cameraForwardX = 0.0f;
  float cameraForwardY = 0.0f;
  float cameraForwardZ = 0.0f;
};

constexpr size_t kFrameContextCapacity = 120;
std::array<FrameContextSample, kFrameContextCapacity> g_frameContext{};
size_t g_frameContextWriteIndex = 0;
size_t g_frameContextCount = 0;

// Fallout: New Vegas 1.4.0.525 player singleton pointer slot.""",
)

replace_once(
    "bool WriteFrameHitchRecord(UInt32 messageType, LONGLONG qpcTicks,",
    """bool WriteHitchContextRecord(UInt32 messageType, LONGLONG hitchQpcTicks,
                             LONGLONG qpcFrequency);
bool WriteFrameHitchRecord(UInt32 messageType, LONGLONG qpcTicks,""",
)

replace_once(
    "void ResetLocationMonitor();\nvoid ResetFrameTimer();",
    "void ResetLocationMonitor();\nvoid ResetHitchContext();\nvoid PushFrameContext(const FrameContextSample& sample);\nvoid ResetFrameTimer();",
)

replace_once(
    "void ResetLocationMonitor() {",
    """void ResetHitchContext() {
  g_frameContextWriteIndex = 0;
  g_frameContextCount = 0;
}

void PushFrameContext(const FrameContextSample& sample) {
  g_frameContext[g_frameContextWriteIndex] = sample;
  g_frameContextWriteIndex =
      (g_frameContextWriteIndex + 1) % kFrameContextCapacity;
  if (g_frameContextCount < kFrameContextCapacity) {
    ++g_frameContextCount;
  }
}

void ResetLocationMonitor() {""",
)

replace_once(
    "  g_previousVelocityZ = 0.0;\n}",
    "  g_previousVelocityZ = 0.0;\n  ResetHitchContext();\n}",
)

replace_once(
    "  const bool isHitch =\n      frameDeltaMilliseconds >= kFrameHitchThresholdMilliseconds;",
    """  UInt32 sampleCellFormId = 0;
  if (player && player->parentCell) {
    sampleCellFormId = player->parentCell->refID;
  }

  CameraTelemetry cameraTelemetry{};
  TryCaptureCameraTelemetry(cameraTelemetry);

  const bool isHitch =
      frameDeltaMilliseconds >= kFrameHitchThresholdMilliseconds;""",
)

replace_once(
    "      CameraTelemetry cameraTelemetry{};\n      TryCaptureCameraTelemetry(cameraTelemetry);\n\n      WriteFrameHitchRecord(",
    """      WriteHitchContextRecord(messageType, performanceCounter.QuadPart,
                              performanceFrequency.QuadPart);

      WriteFrameHitchRecord(""",
)

replace_once(
    "  } else {\n    g_frameHitchActive = false;\n  }\n}\n\nvoid SeedLocationMonitor",
    """  } else {
    g_frameHitchActive = false;
  }

  FrameContextSample contextSample{};
  contextSample.qpcTicks = performanceCounter.QuadPart;
  contextSample.frameDeltaMilliseconds = frameDeltaMilliseconds;
  contextSample.cellFormId = sampleCellFormId;
  contextSample.playerPositionX = playerPositionX;
  contextSample.playerPositionY = playerPositionY;
  contextSample.playerPositionZ = playerPositionZ;
  contextSample.hasVelocity = hasVelocity;
  contextSample.playerSpeed = playerSpeed;
  contextSample.hasAcceleration = hasAcceleration;
  contextSample.accelerationMagnitude = accelerationMagnitude;
  contextSample.hasCameraData = cameraTelemetry.available;
  contextSample.cameraForwardX = cameraTelemetry.forwardX;
  contextSample.cameraForwardY = cameraTelemetry.forwardY;
  contextSample.cameraForwardZ = cameraTelemetry.forwardZ;
  PushFrameContext(contextSample);
}

void SeedLocationMonitor""",
)

context_writer = r'''
bool WriteHitchContextRecord(UInt32 messageType, LONGLONG hitchQpcTicks,
                             LONGLONG qpcFrequency) {
  if (g_frameContextCount == 0 || qpcFrequency <= 0) {
    return false;
  }

  double totalFrameMs = 0.0;
  double maxFrameMs = 0.0;
  double totalSpeed = 0.0;
  double maxSpeed = 0.0;
  double maxAcceleration = 0.0;
  size_t velocitySamples = 0;
  size_t accelerationSamples = 0;
  size_t cameraSamples = 0;
  size_t cellChanges = 0;
  UInt32 previousCell = 0;
  bool previousCellValid = false;
  const FrameContextSample* oldestCamera = nullptr;
  const FrameContextSample* newestCamera = nullptr;
  LONGLONG oldestQpc = hitchQpcTicks;

  const size_t oldestIndex =
      (g_frameContextWriteIndex + kFrameContextCapacity - g_frameContextCount) %
      kFrameContextCapacity;

  for (size_t i = 0; i < g_frameContextCount; ++i) {
    const FrameContextSample& sample =
        g_frameContext[(oldestIndex + i) % kFrameContextCapacity];
    if (i == 0) {
      oldestQpc = sample.qpcTicks;
    }
    totalFrameMs += sample.frameDeltaMilliseconds;
    maxFrameMs = std::max(maxFrameMs, sample.frameDeltaMilliseconds);
    if (sample.hasVelocity) {
      totalSpeed += sample.playerSpeed;
      maxSpeed = std::max(maxSpeed, sample.playerSpeed);
      ++velocitySamples;
    }
    if (sample.hasAcceleration) {
      maxAcceleration =
          std::max(maxAcceleration, sample.accelerationMagnitude);
      ++accelerationSamples;
    }
    if (sample.hasCameraData) {
      if (!oldestCamera) {
        oldestCamera = &sample;
      }
      newestCamera = &sample;
      ++cameraSamples;
    }
    if (sample.cellFormId != 0) {
      if (previousCellValid && sample.cellFormId != previousCell) {
        ++cellChanges;
      }
      previousCell = sample.cellFormId;
      previousCellValid = true;
    }
  }

  double cameraTurnDegrees = 0.0;
  bool hasCameraTurn = oldestCamera && newestCamera && oldestCamera != newestCamera;
  if (hasCameraTurn) {
    double dot =
        static_cast<double>(oldestCamera->cameraForwardX) *
            static_cast<double>(newestCamera->cameraForwardX) +
        static_cast<double>(oldestCamera->cameraForwardY) *
            static_cast<double>(newestCamera->cameraForwardY) +
        static_cast<double>(oldestCamera->cameraForwardZ) *
            static_cast<double>(newestCamera->cameraForwardZ);
    dot = std::clamp(dot, -1.0, 1.0);
    cameraTurnDegrees = std::acos(dot) * (180.0 / 3.14159265358979323846);
  }

  const double windowMilliseconds =
      hitchQpcTicks >= oldestQpc
          ? (static_cast<double>(hitchQpcTicks - oldestQpc) /
             static_cast<double>(qpcFrequency)) * 1000.0
          : 0.0;
  const double averageFrameMs =
      totalFrameMs / static_cast<double>(g_frameContextCount);
  const double averageSpeed =
      velocitySamples > 0 ? totalSpeed / static_cast<double>(velocitySamples)
                          : 0.0;

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
  char line[2048]{};
  const int length = std::snprintf(
      line, sizeof(line),
      "{\"schema\":\"atlas.runtime.event/1\","
      "\"timestampUtc\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ\","
      "\"eventType\":\"hitch_context\",\"messageType\":%u,"
      "\"pluginVersion\":53,\"stage\":23,"
      "\"sampleCount\":%u,\"capacity\":%u,"
      "\"windowMilliseconds\":%.6f,"
      "\"averageFrameMilliseconds\":%.6f,"
      "\"maxFrameMilliseconds\":%.6f,"
      "\"velocitySampleCount\":%u,\"averagePlayerSpeed\":%.6f,"
      "\"maxPlayerSpeed\":%.6f,"
      "\"accelerationSampleCount\":%u,"
      "\"maxAccelerationMagnitude\":%.6f,"
      "\"cameraSampleCount\":%u,\"hasCameraTurn\":%s,"
      "\"cameraTurnDegrees\":%.6f,\"cellChangesInWindow\":%u}\r\n",
      time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
      time.wSecond, time.wMilliseconds, static_cast<unsigned>(messageType),
      static_cast<unsigned>(g_frameContextCount),
      static_cast<unsigned>(kFrameContextCapacity), windowMilliseconds,
      averageFrameMs, maxFrameMs, static_cast<unsigned>(velocitySamples),
      averageSpeed, maxSpeed, static_cast<unsigned>(accelerationSamples),
      maxAcceleration, static_cast<unsigned>(cameraSamples),
      hasCameraTurn ? "true" : "false", cameraTurnDegrees,
      static_cast<unsigned>(cellChanges));

  DWORD written = 0;
  const bool success =
      length > 0 && length < static_cast<int>(sizeof(line)) &&
      WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr) &&
      written == static_cast<DWORD>(length);
  FlushFileBuffers(file);
  CloseHandle(file);
  return success;
}

'''
replace_once("bool WriteFrameHitchRecord(UInt32 messageType, LONGLONG qpcTicks,", context_writer + "bool WriteFrameHitchRecord(UInt32 messageType, LONGLONG qpcTicks,")

replace_once('"pluginVersion":52,', '"pluginVersion":53,')

output_path.parent.mkdir(parents=True, exist_ok=True)
output_path.write_text(text, encoding="utf-8", newline="\n")
print(f"Generated Stage 23 source: {output_path}")
