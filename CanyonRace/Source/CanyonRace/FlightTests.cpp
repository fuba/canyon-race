#include "HoverPawn.h"
#include "CanyonTrack.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Camera/CameraComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

void AHoverPawn::FlightTestTick(float Dt)
{
    FlightTestAge+=Dt;
    if (FlightTestAge<1.) return;
    APlayerController* PC=Cast<APlayerController>(Controller);
    if (!PC) return;
    if (!bCaptured && FlightTestStage==3 && FlightTestAge>1.1 && FParse::Param(FCommandLine::Get(),TEXT("CanyonVernierShot")))
    {
        bCaptured=true;
        // Render-only inspection angle; restore the pilot camera before attitude assertions.
        const FVector View(-900,-650,450);
        Camera->SetRelativeLocationAndRotation(View,(FVector(-5,0,-155)-View).Rotation());
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/VernierReverse.png"),true,false);
    }
    auto Key=[PC](FKey K,bool Down){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,Down?IE_Pressed:IE_Released,Down?1.f:0.f));};
    auto Check=[this](bool OK,const TCHAR* Message)
    {
        UE_LOG(LogTemp,Display,TEXT("CANYON_FLIGHT %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),Message);
        if (!OK)
        {
            FFileHelper::SaveStringToFile(FString(TEXT("FAIL: "))+Message,*(FPaths::ProjectSavedDir()/TEXT("CanyonFlight.txt")));
            bFlightTest=false;FPlatformMisc::RequestExitWithStatus(true,1);
        }
        return OK;
    };
    auto Setup=[&](float Height)
    {
        RestartRace();Race.Start();
        SetActorLocation(Track->Gates[0]+FVector(0,0,Height));
        FlightVelocity=GetActorForwardVector()*6000.;
        FlightTestStart=GetActorLocation();FlightTestYaw=GetActorRotation().Yaw;FlightTestAge=0.;
    };
    switch (FlightTestStage)
    {
    case 0: Setup(15000);FlightVelocity=FVector::ZeroVector;Key(EKeys::W,true);Key(EKeys::Down,true);FlightTestStage=1;break;
    case 1:
        if (FlightTestAge<2.5) return;
        if (!Check(LiftInput>.9f && PitchAngle>83 && GetActorForwardVector().Z>.99 && FlightVelocity.Z>1000,TEXT("pull back points nose nearly vertically up and main thrust climbs from zero airspeed"))) return;
        Key(EKeys::Down,false);Key(EKeys::Up,true);FlightTestStart=GetActorLocation();FlightTestAge=0.;FlightTestStage=2;break;
    case 2:
        if (FlightTestAge<3.) return;
        if (!Check(LiftInput<-.9f && PitchAngle<-83 && GetActorForwardVector().Z<-.99 && FlightVelocity.Z<-1000,TEXT("push forward points nose nearly vertically down and thrust drives descent"))) return;
        Key(EKeys::Up,false);Key(EKeys::W,false);Setup(8000);Key(EKeys::Q,true);FlightTestStage=3;break;
    case 3:
        if (FlightTestAge<1.5) return;
        if (bCaptured) Camera->SetRelativeLocationAndRotation(FVector::ZeroVector,FRotator::ZeroRotator);
        if (!Check(LeftAirBrake>.9f && FMath::FindDeltaAngleDegrees(FlightTestYaw,GetActorRotation().Yaw)<-15 && FlightVelocity.Size2D()<4000,TEXT("left reverse jet slows and yaws left in the air without steering"))) return;
        Key(EKeys::Q,false);Setup(8000);Key(EKeys::E,true);FlightTestStage=4;break;
    case 4:
        if (FlightTestAge<1.5) return;
        if (!Check(RightAirBrake>.9f && FMath::FindDeltaAngleDegrees(FlightTestYaw,GetActorRotation().Yaw)>15 && FlightVelocity.Size2D()<4000,TEXT("right reverse jet slows and yaws right in the air without steering"))) return;
        Key(EKeys::E,false);Setup(8000);Key(EKeys::Q,true);Key(EKeys::E,true);FlightTestStage=5;break;
    case 5:
        if (FlightTestAge<1.5) return;
        if (!Check(LeftAirBrake>.9f && RightAirBrake>.9f && FMath::Abs(FMath::FindDeltaAngleDegrees(FlightTestYaw,GetActorRotation().Yaw))<1 && FlightVelocity.Size2D()<1000,TEXT("both reverse jets cancel yaw and strongly slow the craft"))) return;
        Key(EKeys::Q,false);Key(EKeys::E,false);Setup(0);FlightVelocity=FVector::ZeroVector;Key(EKeys::Down,true);FlightTestStage=6;break;
    case 6:
        if (FlightTestAge<1.5) return;
        if (!Check(PitchAngle>80 && FMath::Abs(GetActorLocation().Z-FlightTestStart.Z)<100,TEXT("attitude jets pitch at rest without translation when main thrust is off"))) return;
        Key(EKeys::Down,false);Setup(15000);Key(EKeys::D,true);FlightTestStage=7;break;
    case 7:
        if (FlightTestAge<1.5) return;
        if (!Check(BankAngle>35 && GetActorRightVector().Z<-.5 && FMath::FindDeltaAngleDegrees(FlightTestYaw,GetActorRotation().Yaw)>15 && FVector::DotProduct(FlightVelocity,FRotator(0,FlightTestYaw,0).RotateVector(FVector::RightVector))>1500,TEXT("right stick banks right wing down and bends airborne trajectory right"))) return;
        if (!Check(FMath::Abs(Camera->GetComponentRotation().Roll-BankAngle)<.1f,TEXT("pilot camera follows cockpit bank"))) return;
        Key(EKeys::D,false);FlightTestAge=0.;FlightTestStage=8;break;
    case 8:
        if (FlightTestAge<1.5) return;
        if (!Check(FMath::Abs(BankAngle)<1 && FMath::Abs(YawSpeed)<3,TEXT("neutral stick smoothly levels the craft and settles the turn"))) return;
        Setup(15000);Key(EKeys::A,true);FlightTestStage=9;break;
    case 9:
        if (FlightTestAge<1.5) return;
        if (!Check(BankAngle<-35 && GetActorRightVector().Z>.5 && FMath::FindDeltaAngleDegrees(FlightTestYaw,GetActorRotation().Yaw)<-15 && FVector::DotProduct(FlightVelocity,FRotator(0,FlightTestYaw,0).RotateVector(FVector::RightVector))<-1500,TEXT("left stick banks left wing down and bends airborne trajectory left"))) return;
        Key(EKeys::A,false);Respawn();
        if (!Check(PitchAngle==0 && BankAngle==0 && YawSpeed==0 && FMath::Abs(GetActorRotation().Roll)<.01f,TEXT("respawn clears bank and turn momentum"))) return;
        Setup(15000);Race=FRaceRules();FlightVelocity=FVector::ZeroVector;Key(EKeys::Q,true);Key(EKeys::E,true);FlightTestStage=10;break;
    case 10:
        if (FlightTestAge<1.5) return;
        if (!Check(Race.Phase==ERacePhase::Racing && FVector::DotProduct(FlightVelocity,GetActorForwardVector())<-3000 && FMath::Abs(FMath::FindDeltaAngleDegrees(FlightTestYaw,GetActorRotation().Yaw))<1,TEXT("symmetric reverse jets start race and accelerate backwards from rest"))) return;
        Key(EKeys::Q,false);Key(EKeys::E,false);
        FFileHelper::SaveStringToFile(TEXT("PASS: near-vertical pitch and nose-directed thrust, rest attitude, independent reverse jets, backwards thrust, bank turns, cockpit attitude, neutral leveling, respawn reset"),*(FPaths::ProjectSavedDir()/TEXT("CanyonFlight.txt")));
        bFlightTest=false;FPlatformMisc::RequestExit(false);break;
    }
}
