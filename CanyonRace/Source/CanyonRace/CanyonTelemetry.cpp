#include "HoverPawn.h"
#include "CanyonMovement.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "HeadMountedDisplayTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "RenderTimer.h"
#include "DynamicRHI.h"

void AHoverPawn::StartTelemetry()
{
    TelemetryDir=FPaths::ProjectSavedDir()/TEXT("Telemetry")/(FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"))+FString::Printf(TEXT("-%u"),FPlatformProcess::GetCurrentProcessId()));
    IFileManager::Get().MakeDirectory(*TelemetryDir,true);
    TelemetryStart=PreviousFrameTime=FPlatformTime::Seconds();
    TArray<FKey> All;EKeys::GetAllKeys(All);
    for (const FKey& Key:All)
    {
        const FString N=Key.GetFName().ToString();
        if ((!Key.IsDeprecated() && Key.IsAxis1D()) || N.StartsWith(TEXT("OculusTouch_")) || N.StartsWith(TEXT("ValveIndex_")) || N.StartsWith(TEXT("SteamFrame_")) || N.StartsWith(TEXT("Gamepad_")) || N==TEXT("W") || N==TEXT("A") || N==TEXT("S") || N==TEXT("D") || N==TEXT("F") || N==TEXT("R") || N==TEXT("LeftShift") || N==TEXT("BackSpace") || N==TEXT("Q") || N==TEXT("E") || N==TEXT("Up") || N==TEXT("Down"))
            if (!Key.IsAxis2D() && !Key.IsAxis3D()) TelemetryKeys.Add(Key);
    }
    APlayerController* PC=Cast<APlayerController>(Controller);
    const FString Meta=FString::Printf(TEXT("CanyonRace telemetry v4\nBuild=%s\nUTC=%s\nPlayerInput=%s\nKeys=%d\nUnits: centimeters, degrees, seconds; frame timing milliseconds\nposes.csv: 20 Hz game-thread poses (not late-render poses), resolved actions and tracking/profile state\ninputs.csv: engine raw-key changes sampled each game tick, analog threshold 0.005\nframes.csv: every game tick wall and simulation intervals\nEach buffer flushes once per second and at normal exit. Last <=1s may be absent after a crash.\nCommandLine=%s\n"),TEXT(CANYON_VERSION),*FDateTime::UtcNow().ToIso8601(),PC && PC->PlayerInput?*PC->PlayerInput->GetClass()->GetName():TEXT("none"),TelemetryKeys.Num(),FCommandLine::Get());
    FFileHelper::SaveStringToFile(Meta,*(TelemetryDir/TEXT("session.txt")));
    FrameBuffer=TEXT("t_s,frame,wall_ms,simulation_ms,game_thread_ms,render_thread_ms,rhi_thread_ms,gpu_ms\n");
    InputBuffer=TEXT("t_s,frame,key,value\n");
    TelemetryBuffer=TEXT("t_s,frame,phase,gate,hp,ships,race_s,throttle,steer,brake,boost,speed_cm_s,vx,vy,vz,px,py,pz,pawn_yaw,head_x,head_y,head_z,head_pitch,head_yaw,head_roll,camera_x,camera_y,camera_z,camera_pitch,camera_yaw,camera_roll,camera_vs_nose_deg,seat_calibrated,hmd_enabled,hmd_tracked,uses_vr_focus,has_vr_focus,left_valid,left_tracking,left_x,left_y,left_z,left_pitch,left_yaw,left_roll,right_valid,right_tracking,right_x,right_y,right_z,right_pitch,right_yaw,right_roll,left_profile,right_profile,origin_revision,calibration_count,recenter_pending,pitch_input,left_reverse,right_reverse,thrust_cm_s2,bank_deg,yaw_rate_deg_s,bank_accel_cm_s2,pawn_pitch,pitch_rate_deg_s,contact_hits,penetration_attempts,requested_move_cm,actual_move_cm,blocked_s,contact_nx,contact_ny,contact_nz\n");
    FlushTelemetry();
    UE_LOG(LogTemp,Display,TEXT("CANYON_TELEMETRY %s"),*FPaths::ConvertRelativePathToFull(TelemetryDir));
}

void AHoverPawn::RecordTelemetry(float Dt)
{
    if (TelemetryDir.IsEmpty()) return;
    const double Now=FPlatformTime::Seconds(),T=Now-TelemetryStart;
    ++TelemetryFrame;
    FrameBuffer+=FString::Printf(TEXT("%.6f,%lld,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n"),T,TelemetryFrame,(Now-PreviousFrameTime)*1000.,Dt*1000.,FPlatformTime::ToMilliseconds(GGameThreadTime),FPlatformTime::ToMilliseconds(GRenderThreadTime),FPlatformTime::ToMilliseconds(GRHIThreadTime),GDynamicRHI?FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()):0.);
    PreviousFrameTime=Now;
    APlayerController* PC=Cast<APlayerController>(Controller);
    if (PC) for (const FKey& K:TelemetryKeys)
    {
        const float V=K.IsAnalog()?PC->GetInputAnalogKeyState(K):(PC->IsInputKeyDown(K)?1.f:0.f);
        float* Old=PreviousKeys.Find(K.GetFName());
        if (!Old || FMath::Abs(*Old-V)>.005f)
        {
            InputBuffer+=FString::Printf(TEXT("%.6f,%lld,%s,%.5f\n"),T,TelemetryFrame,*K.GetFName().ToString(),V);
            PreviousKeys.Add(K.GetFName(),V);
        }
    }
    TelemetryElapsed+=Dt;TelemetryFlushElapsed+=Dt;
    if (TelemetryElapsed>=.05f)
    {
        TelemetryElapsed=0.f;
        FRotator HR;FVector HP;UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HR,HP);
        FXRMotionControllerState L,R;
        UHeadMountedDisplayFunctionLibrary::GetMotionControllerState(this,EXRSpaceType::XRTrackingSpace,EControllerHand::Left,EXRControllerPoseType::Grip,L);
        UHeadMountedDisplayFunctionLibrary::GetMotionControllerState(this,EXRSpaceType::XRTrackingSpace,EControllerHand::Right,EXRControllerPoseType::Grip,R);
        FString LP,RP;UHeadMountedDisplayFunctionLibrary::GetCurrentInteractionProfile(EControllerHand::Left,LP);UHeadMountedDisplayFunctionLibrary::GetCurrentInteractionProfile(EControllerHand::Right,RP);
        bool UsesFocus=false,HasFocus=false;UHeadMountedDisplayFunctionLibrary::GetVRFocusState(UsesFocus,HasFocus);
        FString Row=FString::Printf(TEXT("%.6f,%lld,%d,%d,%.2f,%d,%.3f,%.4f,%.4f,%.4f,%d,%.3f"),T,TelemetryFrame,int32(Race.Phase),Race.LastGate,Race.Health,Race.Ships,Race.Elapsed,Throttle,Steering,Brake,bBoost,FlightVelocity.Size());
        auto Vector=[&Row](const FVector& V){Row+=FString::Printf(TEXT(",%.3f,%.3f,%.3f"),V.X,V.Y,V.Z);};
        auto Rotation=[&Row](const FRotator& V){Row+=FString::Printf(TEXT(",%.3f,%.3f,%.3f"),V.Pitch,V.Yaw,V.Roll);};
        Vector(FlightVelocity);Vector(GetActorLocation());Row+=FString::Printf(TEXT(",%.3f"),GetActorRotation().Yaw);
        Vector(HP);Rotation(HR);Vector(Camera->GetComponentLocation());Rotation(Camera->GetComponentRotation());
        Row+=FString::Printf(TEXT(",%.3f,%d,%d,%d,%d,%d,%d,%d"),FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw,Camera->GetComponentRotation().Yaw),bSeatCalibrated,UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled(),UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition(),UsesFocus,HasFocus,L.bValid,int32(L.TrackingStatus));
        Vector(L.ControllerLocation);Rotation(L.ControllerRotation.Rotator());
        Row+=FString::Printf(TEXT(",%d,%d"),R.bValid,int32(R.TrackingStatus));Vector(R.ControllerLocation);Rotation(R.ControllerRotation.Rotator());
        Row+=TEXT(",")+LP+TEXT(",")+RP+FString::Printf(TEXT(",%llu,%d,%d,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%.3f,%.3f,%.3f"),LastReferenceRevision,SeatCalibrationCount,bRuntimeRecenterPending,LiftInput,ReverseLeft,ReverseRight,AppliedThrust,BankAngle,YawSpeed,BankAcceleration,PitchAngle,PitchRate,ContactHits,Movement->PenetrationAttempts,RequestedMove,ActualMove,BlockedSeconds);
        Vector(ContactNormal);Row+=TEXT("\n");TelemetryBuffer+=Row;
        ContactHits=Movement->PenetrationAttempts=0;RequestedMove=ActualMove=0.f;
    }
    if (TelemetryFlushElapsed>=1.f) {FlushTelemetry();TelemetryFlushElapsed=0.f;}
}

void AHoverPawn::FlushTelemetry()
{
    if (TelemetryDir.IsEmpty()) return;
    auto Flush=[this](FString& Buffer,const TCHAR* Name)
    {
        if (!Buffer.IsEmpty())
        {
            if (!FFileHelper::SaveStringToFile(Buffer,*(TelemetryDir/Name),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_Append|FILEWRITE_AllowRead))
            {
                UE_LOG(LogTemp,Warning,TEXT("CANYON_TELEMETRY_WRITE_FAILED %s"),Name);
            }
            else Buffer.Reset(); // Retry pending records next flush if a reader temporarily locks the file.
        }
    };
    Flush(FrameBuffer,TEXT("frames.csv"));Flush(InputBuffer,TEXT("inputs.csv"));Flush(TelemetryBuffer,TEXT("poses.csv"));
}
