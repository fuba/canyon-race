#include "HoverPawn.h"
#include "CanyonTrack.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"

void AHoverPawn::FlightTestTick(float Dt)
{
    FlightTestAge+=Dt;
    if (FlightTestAge<1.) return;
    APlayerController* PC=Cast<APlayerController>(Controller);
    if (!PC) return;
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
    case 0: Setup(0);Key(EKeys::W,true);Key(EKeys::Down,true);FlightTestStage=1;break;
    case 1:
        if (FlightTestAge<2.) return;
        if (!Check(LiftInput>.9f && GetActorLocation().Z>FlightTestStart.Z+1000 && FlightVelocity.Z>300,TEXT("pull back generates airborne climb through configured input"))) return;
        Key(EKeys::Down,false);Key(EKeys::Up,true);FlightTestStart=GetActorLocation();FlightTestAge=0.;FlightTestStage=2;break;
    case 2:
        if (FlightTestAge<1.) return;
        if (!Check(LiftInput<-.9f && FlightVelocity.Z<-300,TEXT("push forward reverses vertical velocity into descent"))) return;
        Key(EKeys::Up,false);Key(EKeys::W,false);Setup(8000);Key(EKeys::Q,true);FlightTestStage=3;break;
    case 3:
        if (FlightTestAge<1.5) return;
        if (!Check(LeftAirBrake>.9f && FMath::FindDeltaAngleDegrees(FlightTestYaw,GetActorRotation().Yaw)<-15 && FlightVelocity.Size2D()<4000,TEXT("left brake slows and yaws left in the air without steering"))) return;
        Key(EKeys::Q,false);Setup(8000);Key(EKeys::E,true);FlightTestStage=4;break;
    case 4:
        if (FlightTestAge<1.5) return;
        if (!Check(RightAirBrake>.9f && FMath::FindDeltaAngleDegrees(FlightTestYaw,GetActorRotation().Yaw)>15 && FlightVelocity.Size2D()<4000,TEXT("right brake slows and yaws right in the air without steering"))) return;
        Key(EKeys::E,false);Setup(8000);Key(EKeys::Q,true);Key(EKeys::E,true);FlightTestStage=5;break;
    case 5:
        if (FlightTestAge<1.5) return;
        if (!Check(LeftAirBrake>.9f && RightAirBrake>.9f && FMath::Abs(FMath::FindDeltaAngleDegrees(FlightTestYaw,GetActorRotation().Yaw))<1 && FlightVelocity.Size2D()<1000,TEXT("both brakes cancel yaw and strongly slow the craft"))) return;
        Key(EKeys::Q,false);Key(EKeys::E,false);Setup(0);FlightVelocity=FVector::ZeroVector;Key(EKeys::Down,true);FlightTestStage=6;break;
    case 6:
        if (FlightTestAge<1.5) return;
        if (!Check(FMath::Abs(GetActorLocation().Z-FlightTestStart.Z)<100,TEXT("pulling back at rest does not create free vertical thrust"))) return;
        Key(EKeys::Down,false);
        FFileHelper::SaveStringToFile(TEXT("PASS: configured lift/descent inputs, speed-dependent takeoff, left/right airborne brake turns, symmetric braking, no lift at rest"),*(FPaths::ProjectSavedDir()/TEXT("CanyonFlight.txt")));
        bFlightTest=false;FPlatformMisc::RequestExit(false);break;
    }
}
