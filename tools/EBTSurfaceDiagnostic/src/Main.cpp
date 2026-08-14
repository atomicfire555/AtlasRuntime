#include "PCH.hpp"

#include "F4SE/API.hpp"
#include "F4SE/LoadInterface.hpp"
#include "F4SE/MessagingInterface.hpp"
#include "RE/B/bhkPickData.hpp"
#include "RE/C/COL_LAYER.hpp"
#include "RE/H/hknpClosestHitCollector.hpp"
#include "RE/T/TES.hpp"
#include "RE/T/TESDeathEvent.hpp"
#include "RE/T/TESObjectCELL.hpp"
#include "RE/T/TESObjectREFR.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <string_view>

namespace
{
    constexpr float kRayStartHeight = 96.0F;
    constexpr float kRayDepth = 512.0F;
    constexpr float kGridSpacing = 48.0F;

    std::atomic_bool g_registered{ false };

    [[nodiscard]] std::string_view LayerName(RE::COL_LAYER a_layer) noexcept
    {
        using enum RE::COL_LAYER;
        switch (a_layer) {
        case kStatic: return "Static";
        case kAnimStatic: return "AnimStatic";
        case kTransparent: return "Transparent";
        case kClutter: return "Clutter";
        case kTrees: return "Trees";
        case kProps: return "Props";
        case kWater: return "Water";
        case kTerrain: return "Terrain";
        case kGround: return "Ground";
        case kClutterLarge: return "ClutterLarge";
        case kInvisibleWall: return "InvisibleWall";
        case kCollisionBox: return "CollisionBox";
        case kStairHelper: return "StairHelper";
        case kDeadBip: return "DeadBip";
        case kBiped: return "Biped";
        case kBipedNoCC: return "BipedNoCC";
        case kCharController: return "CharController";
        default: return "Other";
        }
    }

    class SurfaceCollector final : public RE::hknpClosestHitCollector
    {
    public:
        void AddHit(const RE::hknpCollisionResult& a_result) override
        {
            const auto* filter = a_result.hitBodyInfo.shapeCollisionFilterInfo.operator->();
            if (!filter) {
                return;
            }

            const auto layer = filter->GetCollisionLayer();
            switch (layer) {
            case RE::COL_LAYER::kStatic:
            case RE::COL_LAYER::kAnimStatic:
            case RE::COL_LAYER::kTransparent:
            case RE::COL_LAYER::kClutter:
            case RE::COL_LAYER::kTrees:
            case RE::COL_LAYER::kProps:
            case RE::COL_LAYER::kTerrain:
            case RE::COL_LAYER::kGround:
            case RE::COL_LAYER::kClutterLarge:
            case RE::COL_LAYER::kInvisibleWall:
            case RE::COL_LAYER::kCollisionBox:
            case RE::COL_LAYER::kStairHelper:
                RE::hknpClosestHitCollector::AddHit(a_result);
                break;
            default:
                break;
            }
        }
    };

    struct SurfaceHit
    {
        bool hit{ false };
        float fraction{ 1.0F };
        RE::NiPoint3 position{};
        RE::hkVector4 normal{};
        RE::COL_LAYER layer{ RE::COL_LAYER::kUnidentified };
        std::uint16_t materialID{ 0xFFFF };
        std::uint32_t bodyID{ 0xFFFFFFFF };
        RE::TESObjectREFR* owner{ nullptr };
        RE::NiAVObject* node{ nullptr };
    };

    [[nodiscard]] SurfaceHit CastSurfaceRay(const RE::NiPoint3& a_origin)
    {
        SurfaceHit out{};

        const RE::NiPoint3 start{ a_origin.x, a_origin.y, a_origin.z + kRayStartHeight };
        const RE::NiPoint3 end{ a_origin.x, a_origin.y, a_origin.z - kRayDepth };

        RE::bhkPickData pickData{};
        SurfaceCollector collector{};
        pickData.collector = std::addressof(collector);
        pickData.collectorType = static_cast<RE::bhkPickData::COLLECTOR_TYPE>(1);
        pickData.castQuery.filterData.collisionFilterInfo->SetCollisionLayer(RE::COL_LAYER::kLOS);
        pickData.SetStartEnd(start, end);

        auto* hitNode = RE::TES::GetSingleton()->Pick(pickData);
        if (!pickData.HasHit()) {
            return out;
        }

        out.hit = true;
        out.fraction = pickData.GetHitFraction();
        out.position = (end - start) * out.fraction + start;
        out.normal = pickData.result.normal;

        if (const auto* filter = pickData.result.hitBodyInfo.shapeCollisionFilterInfo.operator->()) {
            out.layer = filter->GetCollisionLayer();
        }
        out.materialID = pickData.result.hitBodyInfo.shapeMaterialId.value_or(0xFFFF);
        out.bodyID = pickData.result.hitBodyInfo.bodyId.value_or(0xFFFFFFFF);
        out.node = hitNode;
        out.owner = hitNode ? RE::TESObjectREFR::FindReferenceFor3D(hitNode) : nullptr;
        return out;
    }

    void LogSurfaceSample(std::uint32_t a_sampleIndex, float a_dx, float a_dy, const SurfaceHit& a_hit)
    {
        if (!a_hit.hit) {
            REX::INFO("[EBT-SURFACE] sample={} offset=({:.1f},{:.1f}) MISS", a_sampleIndex, a_dx, a_dy);
            return;
        }

        std::uint32_t ownerFormID = 0;
        std::uint32_t baseFormID = 0;
        const char* ownerEditorID = "";
        const char* baseEditorID = "";
        const char* objectType = "";

        if (a_hit.owner) {
            ownerFormID = a_hit.owner->GetFormID();
            ownerEditorID = a_hit.owner->GetFormEditorID();
            if (auto* base = a_hit.owner->GetBaseObject()) {
                baseFormID = base->GetFormID();
                baseEditorID = base->GetFormEditorID();
                objectType = base->GetObjectTypeName();
            }
        }

        const char* nodeName = "";
        if (a_hit.node && a_hit.node->name.c_str()) {
            nodeName = a_hit.node->name.c_str();
        }

        REX::INFO(
            "[EBT-SURFACE] sample={} offset=({:.1f},{:.1f}) hit=({:.2f},{:.2f},{:.2f}) frac={:.5f} normal=({:.4f},{:.4f},{:.4f}) layer={}({}) materialID={} bodyID={} owner={:08X} ownerEDID='{}' base={:08X} baseEDID='{}' type='{}' node='{}'",
            a_sampleIndex,
            a_dx,
            a_dy,
            a_hit.position.x,
            a_hit.position.y,
            a_hit.position.z,
            a_hit.fraction,
            a_hit.normal.x,
            a_hit.normal.y,
            a_hit.normal.z,
            LayerName(a_hit.layer),
            static_cast<std::int32_t>(a_hit.layer),
            a_hit.materialID,
            a_hit.bodyID,
            ownerFormID,
            ownerEditorID ? ownerEditorID : "",
            baseFormID,
            baseEditorID ? baseEditorID : "",
            objectType ? objectType : "",
            nodeName ? nodeName : "");
    }

    void DiagnoseDeath(RE::TESObjectREFR* a_ref, RE::TESObjectREFR* a_killer)
    {
        if (!a_ref) {
            return;
        }

        const auto pos = a_ref->GetPosition();
        const auto base = a_ref->GetBaseObject();
        const auto cell = a_ref->GetParentCell();

        REX::INFO("[EBT-SURFACE] ============================================================");
        REX::INFO(
            "[EBT-SURFACE] DEATH actor={:08X} actorEDID='{}' base={:08X} baseEDID='{}' killer={:08X} pos=({:.2f},{:.2f},{:.2f}) cell={:08X}",
            a_ref->GetFormID(),
            a_ref->GetFormEditorID() ? a_ref->GetFormEditorID() : "",
            base ? base->GetFormID() : 0,
            (base && base->GetFormEditorID()) ? base->GetFormEditorID() : "",
            a_killer ? a_killer->GetFormID() : 0,
            pos.x,
            pos.y,
            pos.z,
            cell ? cell->GetFormID() : 0);

        static constexpr std::array<std::pair<float, float>, 9> offsets{ {
            { 0.0F, 0.0F },
            { -kGridSpacing, -kGridSpacing },
            { 0.0F, -kGridSpacing },
            { kGridSpacing, -kGridSpacing },
            { -kGridSpacing, 0.0F },
            { kGridSpacing, 0.0F },
            { -kGridSpacing, kGridSpacing },
            { 0.0F, kGridSpacing },
            { kGridSpacing, kGridSpacing }
        } };

        std::uint32_t index = 0;
        for (const auto& [dx, dy] : offsets) {
            RE::NiPoint3 origin{ pos.x + dx, pos.y + dy, pos.z };
            LogSurfaceSample(index++, dx, dy, CastSurfaceRay(origin));
        }
        REX::INFO("[EBT-SURFACE] END DEATH");
    }

    class DeathSink final : public RE::BSTEventSink<RE::TESDeathEvent>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent& a_event, RE::BSTEventSource<RE::TESDeathEvent>*) override
        {
            if (a_event.dying && a_event.dyingActorRef) {
                DiagnoseDeath(a_event.dyingActorRef.get(), a_event.killerActorRef.get());
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    DeathSink g_deathSink{};

    void RegisterDeathSink()
    {
        if (g_registered.exchange(true)) {
            return;
        }

        auto* source = RE::TESDeathEvent::GetEventSource();
        if (!source || !source->RegisterSink(std::addressof(g_deathSink))) {
            g_registered = false;
            REX::ERROR("[EBT-SURFACE] Failed to register TESDeathEvent sink");
            return;
        }

        REX::INFO("[EBT-SURFACE] TESDeathEvent sink registered");
    }

    void MessageHandler(F4SE::MessagingInterface::Message* a_message)
    {
        if (!a_message) {
            return;
        }

        if (a_message->type == F4SE::MessagingInterface::kGameDataReady ||
            a_message->type == F4SE::MessagingInterface::kGameLoaded) {
            RegisterDeathSink();
        }
    }
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
    F4SE::Init(a_f4se);

    const auto runtime = a_f4se->GetRuntimeVersion();
    const auto f4se = a_f4se->GetF4SEVersion();
    REX::INFO("[EBT-SURFACE] EBTSurfaceDiagnostic 1.0.0 starting");
    REX::INFO("[EBT-SURFACE] Runtime={} F4SE={}", runtime, f4se);
    REX::INFO("[EBT-SURFACE] RayStartHeight={} RayDepth={} GridSpacing={}", kRayStartHeight, kRayDepth, kGridSpacing);

    const auto* messaging = F4SE::GetMessagingInterface().get();
    if (!messaging->RegisterListener(MessageHandler)) {
        REX::ERROR("[EBT-SURFACE] Failed to register F4SE messaging listener");
        return false;
    }

    return true;
}
