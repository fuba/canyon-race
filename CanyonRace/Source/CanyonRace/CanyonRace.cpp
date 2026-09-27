#include "Modules/ModuleManager.h"
#include "IOpenXRExtensionPlugin.h"
#include "CanyonXRRecenter.h"
#include "IOpenXRHMDModule.h"
#include "InputCoreTypes.h"

static FCanyonReferenceChanges ReferenceChanges;
uint64 CanyonAppliedReferenceRevision() {return ReferenceChanges.Revision;}

class FCanyonRaceModule : public FDefaultGameModuleImpl, public IOpenXRExtensionPlugin
{
public:
    virtual void StartupModule() override
    {
        // Register before OpenXR builds legacy actions. Native shoulder buttons have no Touch equivalent.
        for (const TCHAR* Hand:{TEXT("Left"),TEXT("Right")})
        {
            for (const TCHAR* Axis:{TEXT("Trigger_Axis"),TEXT("Thumbstick_X"),TEXT("Thumbstick_Y")})
            {
                const FString Name=FString::Printf(TEXT("SteamFrame_%s_%s"),Hand,Axis);
                EKeys::AddKey(FKeyDetails(FKey(*Name),FText::FromString(Name),FKeyDetails::GamepadKey|FKeyDetails::Axis1D|FKeyDetails::NotBlueprintBindableKey));
            }
            for (const TCHAR* Button:{TEXT("Shoulder_Click"),TEXT("Thumbstick_Click"),TEXT("A_Click"),TEXT("B_Click"),TEXT("X_Click"),TEXT("Y_Click")})
            {
                if (FString(Hand)==TEXT("Left") && !FString(Button).StartsWith(TEXT("Shoulder")) && !FString(Button).StartsWith(TEXT("Thumbstick"))) continue;
                const FString Name=FString::Printf(TEXT("SteamFrame_%s_%s"),Hand,Button);
                EKeys::AddKey(FKeyDetails(FKey(*Name),FText::FromString(Name),FKeyDetails::GamepadKey|FKeyDetails::NotBlueprintBindableKey));
            }
        }
        RegisterOpenXRExtensionModularFeature();
    }
    virtual void ShutdownModule() override {UnregisterOpenXRExtensionModularFeature();}
    virtual FString GetDisplayName() override {return TEXT("CanyonRace seated recenter");}
    virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& Out) override
    {
        Out.Add("XR_VALVE_frame_controller_interaction");return true;
    }
    virtual bool GetInteractionProfiles(XrInstance Instance,TArray<FString>& Prefixes,TArray<XrPath>& Paths,TArray<bool>& Haptics) override
    {
        if (!IOpenXRHMDModule::Get().IsExtensionEnabled(TEXT("XR_VALVE_frame_controller_interaction"))) return false;
        const XrPath Path=IOpenXRHMDModule::Get().ResolveNameToPath(FName(TEXT("/interaction_profiles/valve/frame_controller_valve")));
        if (Path==XR_NULL_PATH) return false;
        Prefixes.Add(TEXT("SteamFrame"));Paths.Add(Path);Haptics.Add(true);
        UE_LOG(LogTemp,Display,TEXT("CANYON_FRAME_NATIVE enabled=1 shoulder_buttons=1"));
        return true;
    }
    virtual void OnEvent(XrSession Session,const XrEventDataBaseHeader* Header) override
    {
        if (Header->type!=XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING) return;
        const auto& Change=*reinterpret_cast<const XrEventDataReferenceSpaceChangePending*>(Header);
        UE_LOG(LogTemp,Display,TEXT("CANYON_XR_SPACE_EVENT type=%d change_time=%lld pose_valid=%d"),int32(Change.referenceSpaceType),Change.changeTime,Change.poseValid);
        // The pawn explicitly uses LOCAL. STAGE/LOCAL_FLOOR notifications must not recenter it again.
        if (Change.referenceSpaceType!=XR_REFERENCE_SPACE_TYPE_LOCAL) return;
        ReferenceChanges.Queue(Change.changeTime);
        UE_LOG(LogTemp,Display,TEXT("CANYON_XR_ORIGIN_PENDING change_time=%lld pose_valid=%d"),Change.changeTime,Change.poseValid);
    }
    virtual void UpdateDeviceLocations(XrSession Session,XrTime DisplayTime,XrSpace Space) override
    {
        // UE has updated its game-thread pose cache for DisplayTime before invoking this hook.
        if (ReferenceChanges.Advance(DisplayTime))
        {
            UE_LOG(LogTemp,Display,TEXT("CANYON_XR_ORIGIN_APPLIED revision=%llu display_time=%lld"),ReferenceChanges.Revision,DisplayTime);
        }
    }
    virtual void OnDestroySession(XrSession Session) override {ReferenceChanges.PendingTimes.Reset();}
};
IMPLEMENT_PRIMARY_GAME_MODULE(FCanyonRaceModule, CanyonRace, "CanyonRace");
