#include "HoverPawn.h"
#include "CanyonTrack.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UnrealClient.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "SeatCalibration.h"
#include "CanyonXRRecenter.h"
#include "FlightControls.h"

AHoverPawn::AHoverPawn()
{
    PrimaryActorTick.bCanEverTick=true;
    Hull=CreateDefaultSubobject<USphereComponent>(TEXT("Hull"));
    Hull->InitSphereRadius(90.f);
    Hull->SetCollisionProfileName(TEXT("Pawn"));
    Hull->SetSimulatePhysics(false);
    RootComponent=Hull;
    SeatTrackingRoot=CreateDefaultSubobject<USceneComponent>(TEXT("SeatTrackingOrigin"));
    SeatTrackingRoot->SetupAttachment(RootComponent);SeatTrackingRoot->SetRelativeLocation(SeatEye);
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("PilotCamera"));
    Camera->SetupAttachment(SeatTrackingRoot);
    Camera->FieldOfView=95.f; Camera->bLockToHmd=true; Camera->bUsePawnControlRotation=false;
    Dashboard=CreateDefaultSubobject<UTextRenderComponent>(TEXT("InWorldDashboard"));
    Dashboard->SetupAttachment(RootComponent); Dashboard->SetRelativeLocation(FVector(251,0,103));
    Dashboard->SetRelativeRotation(FRotator(0,180,0));
    Dashboard->SetHorizontalAlignment(EHTA_Center); Dashboard->SetWorldSize(4.f);
    Dashboard->SetVerticalAlignment(EVRTA_TextTop);
    Dashboard->SetTextRenderColor(FColor(90,230,255));
    const ConstructorHelpers::FObjectFinder<UStaticMesh> Model(TEXT("/Game/Canyon/Models/SM_Cockpit.SM_Cockpit"));
    UStaticMeshComponent* Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cockpit"));
    Body->SetupAttachment(RootComponent);Body->SetStaticMesh(Model.Object);
    Body->SetCollisionProfileName(TEXT("NoCollision"));
    EngineAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
    EngineAudio->SetupAttachment(RootComponent);EngineAudio->bAutoActivate=false;
}

void AHoverPawn::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp,Display,TEXT("CANYON_VERSION %s"),TEXT(CANYON_VERSION));
    for (TActorIterator<ACanyonTrack> It(GetWorld()); It; ++It) { Track=*It; break; }
    if (!Track) Track=GetWorld()->SpawnActor<ACanyonTrack>();
    if (!Track || !Track->EnsureBuilt())
    {
        UE_LOG(LogTemp, Error, TEXT("CANYON_START_FAILED: run scripts/prepare-prototype.py"));
        SetActorTickEnabled(false); return;
    }
    UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Local);
    LastReferenceRevision=CanyonAppliedReferenceRevision();
    Dashboard->SetVisibility(true);
    EngineAudio->SetSound(LoadObject<USoundBase>(nullptr,TEXT("/Game/Canyon/Audio/Engine.Engine")));
    EngineAudio->SetVolumeMultiplier(.12f);EngineAudio->Play();
    ImpactSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/Canyon/Audio/Impact.Impact"));
    GateSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/Canyon/Audio/Gate.Gate"));
    UE_LOG(LogTemp,Display,TEXT("CANYON_VR enabled=%d connected=%d"),UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled(),UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayConnected());
    if (APlayerController* PC=Cast<APlayerController>(Controller))
    {
        PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor=false;
    }
    bSmoke=FParse::Param(FCommandLine::Get(),TEXT("CanyonSmokeTest"));
    bCourseTest=FParse::Param(FCommandLine::Get(),TEXT("CanyonCourseTest"));
    bFlightTest=FParse::Param(FCommandLine::Get(),TEXT("CanyonFlightTest"));
    LoadRecord();
    RestartRace();
    StartTelemetry();
    int32 CaptureGate=0;
    if (FParse::Value(FCommandLine::Get(),TEXT("CanyonCaptureGate="),CaptureGate) && Track->Gates.IsValidIndex(CaptureGate))
        SetActorTransform(Track->SpawnAt(CaptureGate),false,nullptr,ETeleportType::TeleportPhysics);
    UE_LOG(LogTemp, Display, TEXT("CANYON_PAWN_READY pos=%s"),*GetActorLocation().ToString());
}

void AHoverPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("CanyonThrottle"),this,&AHoverPawn::SetThrottle);
    Input->BindAxis(TEXT("CanyonSteer"),this,&AHoverPawn::SetSteering);
    Input->BindAxis(TEXT("CanyonBrake"),this,&AHoverPawn::SetBrake);
    Input->BindAxis(TEXT("CanyonLift"),this,&AHoverPawn::SetLift);
    Input->BindAxis(TEXT("CanyonLeftAirBrake"),this,&AHoverPawn::SetLeftAirBrake);
    Input->BindAxis(TEXT("CanyonRightAirBrake"),this,&AHoverPawn::SetRightAirBrake);
    Input->BindAction(TEXT("CanyonBoost"),IE_Pressed,this,&AHoverPawn::BoostOn);
    Input->BindAction(TEXT("CanyonBoost"),IE_Released,this,&AHoverPawn::BoostOff);
    Input->BindAction(TEXT("CanyonRestart"),IE_Pressed,this,&AHoverPawn::RestartRace);
    Input->BindAction(TEXT("CanyonRecover"),IE_Pressed,this,&AHoverPawn::Recover);
    Input->BindAction(TEXT("CanyonRecenter"),IE_Pressed,this,&AHoverPawn::Recenter);
    Input->BindAction(TEXT("CanyonQuit"),IE_Pressed,this,&AHoverPawn::Quit);
    Input->BindAction(TEXT("CanyonMute"),IE_Pressed,this,&AHoverPawn::ToggleSound);
}

void AHoverPawn::RestartRace()
{
    Race=FRaceRules(); Throttle=Steering=Brake=LiftInput=LeftAirBrake=RightAirBrake=0.f; bBoost=false;
    TimeTrial.StartRun();bSaveRecordFailed=false;
    Respawn(); UpdateDashboard();
}

void AHoverPawn::Respawn()
{
    if (!Track || Track->Gates.IsEmpty()) return;
    SetActorTransform(Track->SpawnAt(Race.LastGate),false,nullptr,ETeleportType::TeleportPhysics);
    FlightVelocity=FVector::ZeroVector; YawSpeed=0.f; FixedRemainder=0.f;
    BankAngle=BankAcceleration=AppliedLift=0.f;
    UE_LOG(LogTemp, Display, TEXT("CANYON_RESPAWN ships=%d gate=%d"),Race.Ships,Race.LastGate);
}

void AHoverPawn::Recover() { Race.Start(); Race.DestroyShip(); FlightVelocity=FVector::ZeroVector; }
void AHoverPawn::Recenter()
{
    if (ApplySeatCalibration(TEXT("manual"))) bCalibrateOnFirstThrottle=false;
}

bool AHoverPawn::ApplySeatCalibration(const TCHAR* Reason)
{
    if (!UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled() || !UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition()) return false;
    bool UsesFocus=false,HasFocus=false;
    UHeadMountedDisplayFunctionLibrary::GetVRFocusState(UsesFocus,HasFocus);
    if (UsesFocus && !HasFocus) return false; // Steam overlays may leave a valid but frozen head pose.
    FRotator HeadRotation;FVector HeadPosition;
    UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HeadRotation,HeadPosition);
    SeatTrackingRoot->SetRelativeTransform(CanyonSeatCalibration(HeadPosition,HeadRotation.Yaw,SeatEye));
    bSeatCalibrated=true;
    ++SeatCalibrationCount;
    UE_LOG(LogTemp,Display,TEXT("CANYON_RECENTER reason=%s count=%d origin_revision=%llu head=%s yaw=%.2f seat=%s"),Reason,SeatCalibrationCount,LastReferenceRevision,*HeadPosition.ToString(),HeadRotation.Yaw,*SeatEye.ToString());
    return true;
}
void AHoverPawn::Quit() { UKismetSystemLibrary::QuitGame(this,Cast<APlayerController>(Controller),EQuitPreference::Quit,false); }
void AHoverPawn::ToggleSound() { bMuted=!bMuted; }

void AHoverPawn::Tick(float Dt)
{
    Super::Tick(Dt);
    if (!Track) return;
    const uint64 ReferenceRevision=CanyonAppliedReferenceRevision();
    if (ReferenceRevision!=LastReferenceRevision)
    {
        LastReferenceRevision=ReferenceRevision;bRuntimeRecenterPending=true;
    }
    if (bRuntimeRecenterPending && ApplySeatCalibration(TEXT("runtime_origin")))
    {
        bRuntimeRecenterPending=false;bCalibrateOnFirstThrottle=false;
    }
    if (!bSeatCalibrated && UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
    {
        bool UsesFocus=false,HasFocus=false;
        UHeadMountedDisplayFunctionLibrary::GetVRFocusState(UsesFocus,HasFocus);
        TrackingStableTime=UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition() && (!UsesFocus || HasFocus)?TrackingStableTime+Dt:0.f;
        if (TrackingStableTime>.5f) ApplySeatCalibration(TEXT("startup"));
    }
    if (bCalibrateOnFirstThrottle && Throttle>.05f && ApplySeatCalibration(TEXT("first_throttle"))) bCalibrateOnFirstThrottle=false;
    if (!bCaptured && GetGameTimeSinceCreation()>3.f && FParse::Param(FCommandLine::Get(),TEXT("CanyonCaptureShot")))
    {
        bCaptured=true;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/CanyonPrototype.png"),true,false);
    }
    if (bSmoke) SmokeTick(Dt);
    if (bCourseTest) CourseTick(Dt);
    if (bFlightTest) FlightTestTick(Dt);
    if (FParse::Param(FCommandLine::Get(),TEXT("CanyonBenchmark"))) BenchmarkTick();
    if (bCaptured && GetGameTimeSinceCreation()>8.f && FParse::Param(FCommandLine::Get(),TEXT("CanyonCaptureExit"))) FPlatformMisc::RequestExit(false);
    if (Throttle>.05f) Race.Start();
    if (Race.Tick(Dt)) Respawn();
    if (Race.Phase==ERacePhase::Ready || Race.Phase==ERacePhase::Racing)
    {
        // Bounded fixed substeps make steering/suspension independent of render rate.
        FixedRemainder+=FMath::Min(Dt,.1f);
        constexpr float Step=1.f/120.f;
        while (FixedRemainder>=Step)
        {
            MoveStep(Step); FixedRemainder-=Step;
            if (Race.Phase!=ERacePhase::Ready && Race.Phase!=ERacePhase::Racing) { FixedRemainder=0; break; }
        }
    }
    else FlightVelocity=FVector::ZeroVector;
    EngineAudio->SetPitchMultiplier(.65f+FMath::Clamp(float(FlightVelocity.Size2D()/MaxSpeed),0.f,1.2f)*1.2f);
    EngineAudio->SetVolumeMultiplier(bMuted?0.f:(Race.Phase==ERacePhase::Racing?.18f+Throttle*.20f:.07f));
    DashElapsed+=Dt;
    if (DashElapsed>.1f) { UpdateDashboard(); DashElapsed=0.f; }
    RecordTelemetry(Dt);
}

void AHoverPawn::EndPlay(const EEndPlayReason::Type Reason)
{
    FlushTelemetry();Super::EndPlay(Reason);
}

void AHoverPawn::BenchmarkTick()
{
    const double Now=FPlatformTime::Seconds();
    if (BenchmarkStart==0.) { BenchmarkStart=BenchmarkPrevious=Now;return; }
    if (Now-BenchmarkStart>5.) FrameSamples.Add(float((Now-BenchmarkPrevious)*1000.));
    BenchmarkPrevious=Now;
    if (Now-BenchmarkStart<20.) return;
    FrameSamples.Sort();double Total=0.;for (float Sample:FrameSamples) Total+=Sample;
    if (!FrameSamples.IsEmpty())
    {
        const double Mean=Total/FrameSamples.Num();
        const FString Result=FString::Printf(TEXT("Desktop frame intervals, 5s warmup + 15s stationary view; frames=%d mean_ms=%.3f p95_ms=%.3f p99_ms=%.3f mean_fps=%.2f. Not a VR performance measurement."),FrameSamples.Num(),Mean,FrameSamples[FMath::FloorToInt((FrameSamples.Num()-1)*.95)],FrameSamples[FMath::FloorToInt((FrameSamples.Num()-1)*.99)],1000./Mean);
        FFileHelper::SaveStringToFile(Result,*(FPaths::ProjectSavedDir()/TEXT("CanyonBenchmark.txt")));
        UE_LOG(LogTemp,Display,TEXT("%s"),*Result);
    }
    FPlatformMisc::RequestExit(false);
}

void AHoverPawn::CourseTick(float Dt)
{
    // Validation driver uses the same thrust, steering, collision and gate detection as a player.
    CourseAge+=Dt;
    if (CourseAge<1.) return;
    Race.Start();
    if (Race.Phase==ERacePhase::Finished)
    {
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("PASS: all %d gates crossed through physical flight; %.2f seconds; %d ships; %.1f hull"),Race.LastGate,Race.Elapsed,Race.Ships,Race.Health),*(FPaths::ProjectSavedDir()/TEXT("CanyonCourse.txt")));
        bCourseTest=false; FPlatformMisc::RequestExit(false);return;
    }
    if (CourseAge>420. || Race.Phase==ERacePhase::Retired)
    {
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("FAIL: gate %d, ships %d, age %.1f, pos %s"),Race.LastGate,Race.Ships,CourseAge,*GetActorLocation().ToString()),*(FPaths::ProjectSavedDir()/TEXT("CanyonCourse.txt")));
        bCourseTest=false;FPlatformMisc::RequestExitWithStatus(true,1);return;
    }
    if (Race.Phase!=ERacePhase::Racing) return;
    const int32 Next=Race.LastGate+1;
    if (!Track->Gates.IsValidIndex(Next)) return;
    FVector Delta=Track->Gates[Next]+Track->GateForward(Next)*1200.-GetActorLocation();Delta.Z=0;
    const FVector Desired=Delta.GetSafeNormal()*2800.;
    FVector Accel=(Desired-FVector(FlightVelocity.X,FlightVelocity.Y,0))*.85;
    Accel+=FVector(FlightVelocity.X,FlightVelocity.Y,0)*.14;
    const float Angle=FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw,Accel.Rotation().Yaw);
    Steering=FMath::Clamp(Angle/28.f,-1.f,1.f);
    Throttle=FMath::Clamp(float(Accel.Size()/Thrust),0.f,1.f)*FMath::Max(0.f,FMath::Cos(FMath::DegreesToRadians(Angle)));
    // Banking needs airflow: keep flying through large heading corrections instead
    // of stopping and trying to pivot in place like the old direct-yaw controller.
    if (FlightVelocity.Size2D()<2200.) Throttle=FMath::Max(Throttle,.3f);
    Brake=FMath::Abs(Angle)>75.f && FlightVelocity.Size2D()>2400.?.4f:0.f;bBoost=false;
}

void AHoverPawn::MoveStep(float Dt)
{
    const FVector Before=GetActorLocation();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HoverGround),true,this);
    FHitResult Ground;
    bool Grounded=GetWorld()->LineTraceSingleByChannel(Ground,Before,Before-FVector(0,0,1600),ECC_Visibility,Params)
        && Ground.ImpactNormal.Z>.35;
    const float ForwardSpeed=FVector::DotProduct(FlightVelocity,GetActorForwardVector());
    const float CombinedBrake=FMath::Clamp(Brake+(LeftAirBrake+RightAirBrake)*.5f,0.f,1.f);
    const bool Active=Race.Phase==ERacePhase::Racing;
    // Stick commands bank, not yaw. Smooth roll-in/roll-out retains a little turn after release.
    const float RollStep=FMath::Clamp((Steering*MaxBankAngle-BankAngle)*4.f,-90.f,90.f);
    BankAngle+=RollStep*Dt;
    AppliedLift=Active?FCanyonAerodynamics::Lift(LiftInput,ForwardSpeed):0.f;
    BankAcceleration=FCanyonAerodynamics::BankAcceleration(BankAngle,ForwardSpeed);
    const float WingSideAcceleration=AppliedLift*FMath::Sin(FMath::DegreesToRadians(BankAngle));
    const float BankTurn=FMath::Clamp(FCanyonAerodynamics::BankYaw(BankAcceleration+WingSideAcceleration,ForwardSpeed),-TurnRate,TurnRate);
    YawSpeed=FMath::FInterpTo(YawSpeed,BankTurn+FCanyonAerodynamics::BrakeYaw(LeftAirBrake,RightAirBrake,ForwardSpeed),Dt,3.f);
    SetActorRotation(FRotator(0,GetActorRotation().Yaw+YawSpeed*Dt,BankAngle));
    const FRotator Heading(0,GetActorRotation().Yaw,0);
    const FVector Forward=Heading.Vector(), Right=Heading.RotateVector(FVector::RightVector);
    FVector Accel(0,0,-980);
    if (Grounded)
    {
        const float Distance=Before.Z-Ground.ImpactPoint.Z;
        // Positive lift only: leaving a cliff never pulls the craft down to the valley floor.
        const float SurfaceVelocity=FVector::DotProduct(FlightVelocity,Ground.ImpactNormal)/Ground.ImpactNormal.Z;
        const float Lift=FMath::Clamp(980.f+(HoverHeight-Distance)*12.f-SurfaceVelocity*6.f,0.f,6500.f);
        Accel.Z+=Lift*(1.f+FMath::Min(0.f,LiftInput));
    }
    // Hover suspension stays vertical. Aerodynamic lift banks with the craft, so pulling
    // through a turn tightens the arc at the cost of some vertical lift.
    Accel+=Right*BankAcceleration+GetActorUpVector()*AppliedLift;
    Accel.Z-=FlightVelocity.Z*.85f*FMath::Abs(LiftInput);
    if (Active) Accel+=Forward*(Throttle*Thrust*(bBoost?1.65f:1.f));
    const FVector Horizontal(FlightVelocity.X,FlightVelocity.Y,0);
    Accel-=Horizontal*(.12f+.000006f*Horizontal.Size()+CombinedBrake*2.0f+FMath::Abs(LiftInput)*.10f);
    Accel-=Right*FVector::DotProduct(FlightVelocity,Right)*(LateralGrip+CombinedBrake*.55f)*(Grounded?1.f:.2f);
    FlightVelocity+=Accel*Dt;
    const double Speed=FlightVelocity.Size2D();
    const double Limit=MaxSpeed*(bBoost?1.2:1.);
    if (Speed>Limit) { FlightVelocity.X*=Limit/Speed; FlightVelocity.Y*=Limit/Speed; }
    FHitResult Hit;
    AddActorWorldOffset(FlightVelocity*Dt,true,&Hit);
    if (Hit.bBlockingHit)
    {
        const double Inward=FMath::Max(0.,-FVector::DotProduct(FlightVelocity,Hit.Normal));
        if (Inward>0.)
        {
            FlightVelocity+=Hit.Normal*Inward*1.4;
            if (Race.Hit(Inward))
            {
                UE_LOG(LogTemp, Display,TEXT("CANYON_IMPACT speed=%.1f hp=%.1f"),Inward,Race.Health);
                if (!bMuted && ImpactSound) UGameplayStatics::PlaySound2D(this,ImpactSound,.65f);
            }
        }
        if (Hit.bStartPenetrating) AddActorWorldOffset(Hit.Normal*(Hit.PenetrationDepth+2.f),false);
    }
    const FVector After=GetActorLocation();
    const int32 Next=Race.LastGate+1;
    if (Track->Gates.IsValidIndex(Next) && CrossedRaceGate(Before,After,Track->Gates[Next],Track->GateForward(Next)))
    {
        Race.PassGate(Next,Track->Gates.Num());
        if (Race.LastGate==Next && TimeTrial.Gate(Next,Track->Gates.Num()-1,Race.Elapsed)) SaveRecord();
        if (!bMuted && GateSound) UGameplayStatics::PlaySound2D(this,GateSound,.6f);
        UE_LOG(LogTemp, Display,TEXT("CANYON_GATE %d time=%.2f"),Next,Race.Elapsed);
    }
    if (FMath::Abs(After.X)>ACanyonTrack::Extent-100 || FMath::Abs(After.Y)>ACanyonTrack::Extent-100 || After.Z<-10000)
    {
        Race.Start(); Race.DestroyShip();
    }
}

FString AHoverPawn::StatusText() const
{
    switch (Race.Phase)
    {
        case ERacePhase::Ready: return TEXT("READY / THROTTLE TO START");
        case ERacePhase::Respawning: return FString::Printf(TEXT("REBUILD IN %.1f s"),Race.RespawnRemaining);
        case ERacePhase::Finished: return TEXT("FINISH / B OR R TO RESTART");
        case ERacePhase::Retired: return TEXT("DNF / B OR R TO RESTART");
        default: return Track?Track->CourseName:TEXT("TIME ATTACK");
    }
}

void AHoverPawn::UpdateDashboard()
{
    const int32 Next=Race.LastGate+1;
    FString Nav;
    if (Track && Track->Gates.IsValidIndex(Next))
    {
        const FVector Delta=Track->Gates[Next]-GetActorLocation();
        const float Angle=FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw,Delta.Rotation().Yaw);
        Nav=FString::Printf(TEXT("GATE %02d   %.0f m   %s %.0f deg"),Next,Delta.Size()/100.f,
            Angle<0?TEXT("LEFT"):TEXT("RIGHT"),FMath::Abs(Angle));
    }
    Dashboard->SetText(FText::FromString(FString::Printf(TEXT("%03.0f km/h   HULL %03.0f   SHIPS %d\n%s\n%s\n%.2f s   V/S %+.1f m/s  L%.0f R%.0f\n%s"),
        FlightVelocity.Size()/100.*3.6,Race.Health,Race.Ships,*Nav,*StatusText(),Race.Elapsed,FlightVelocity.Z/100.,LeftAirBrake*100,RightAirBrake*100,*RecordText())));
}

void AHoverPawn::SmokeAssert(bool Condition, const TCHAR* Message)
{
    UE_LOG(LogTemp, Display,TEXT("CANYON_SMOKE %s %s"),Condition?TEXT("PASS"):TEXT("FAIL"),Message);
    if (!Condition)
    {
        FFileHelper::SaveStringToFile(FString(TEXT("FAIL: "))+Message,*(FPaths::ProjectSavedDir()/TEXT("CanyonSmoke.txt")));
        FPlatformMisc::RequestExitWithStatus(true,1);
    }
}

void AHoverPawn::SmokeTick(float Dt)
{
    SmokeAge+=Dt;
    switch (SmokeStage)
    {
    case 0:
        if (SmokeAge<1.) return;
        SmokeAssert(Track->Gates.Num()>10,TEXT("real terrain and ordered gates loaded"));
        SmokeAssert(FMath::Abs(GetActorLocation().Z-Track->HeightAt(GetActorLocation().X,GetActorLocation().Y)-HoverHeight)<40,
            TEXT("hover suspension settles above collidable terrain"));
        if (APlayerController* PC=Cast<APlayerController>(Controller)) PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W,IE_Pressed,1.f));
        SmokeStage=1; SmokeDeadline=SmokeAge+2.; break;
    case 1:
        if (SmokeAge<SmokeDeadline) return;
        UE_LOG(LogTemp,Display,TEXT("CANYON_ACCEL speed=%.1f phase=%d pos=%s"),FlightVelocity.Size2D(),int32(Race.Phase),*GetActorLocation().ToString());
        SmokeAssert(Race.Phase==ERacePhase::Racing && FlightVelocity.Size2D()>2500,TEXT("configured W input starts race and accelerates craft"));
        if (APlayerController* PC=Cast<APlayerController>(Controller)) PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W,IE_Released,0.f));
        {
            const FVector F=GetActorForwardVector();
            SmokeWall=GetWorld()->SpawnActor<AActor>();
            UStaticMeshComponent* Wall=NewObject<UStaticMeshComponent>(SmokeWall);
            SmokeWall->SetRootComponent(Wall);
            Wall->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
            Wall->SetCollisionProfileName(TEXT("BlockAll")); Wall->RegisterComponent();
            SmokeWall->SetActorLocation(GetActorLocation()+F*2200);
            SmokeWall->SetActorRotation(GetActorRotation()); SmokeWall->SetActorScale3D(FVector(2,100,100));
            FlightVelocity=F*10000.; Throttle=0.; SmokeStage=2; SmokeDeadline=SmokeAge+1.;
        }
        break;
    case 2:
        Throttle=0;
        if (SmokeAge<SmokeDeadline) return;
        SmokeAssert(Race.Health>0 && Race.Health<100,TEXT("swept wall impact damages without instant destruction"));
        SmokeAssert(FVector::DotProduct(FlightVelocity,GetActorForwardVector())<0,TEXT("wall impact rebounds"));
        SmokeWall->Destroy(); Recover();
        SmokeAssert(Race.Ships==2 && Race.Phase==ERacePhase::Respawning,TEXT("first destruction consumes one ship"));
        SmokeStage=3; SmokeDeadline=SmokeAge+4.8; break;
    case 3:
        if (SmokeAge<SmokeDeadline) return;
        SmokeAssert(Race.Phase==ERacePhase::Respawning,TEXT("no early respawn"));
        SmokeStage=4; SmokeDeadline=SmokeAge+.4; break;
    case 4:
        if (SmokeAge<SmokeDeadline) return;
        SmokeAssert(Race.Phase==ERacePhase::Racing && Race.Health==100 && Race.Ships==2,TEXT("respawn restores hull after five seconds"));
        SmokeAssert(FVector::Dist(GetActorLocation(),Track->SpawnAt(Race.LastGate).GetLocation())<100,TEXT("respawn returns to checkpoint"));
        Recover(); SmokeStage=5; SmokeDeadline=SmokeAge+5.2; break;
    case 5:
        if (SmokeAge<SmokeDeadline) return;
        SmokeAssert(Race.Phase==ERacePhase::Racing && Race.Ships==1,TEXT("second respawn uses final ship"));
        Recover(); SmokeAssert(Race.Phase==ERacePhase::Retired && Race.Ships==0,TEXT("third destruction retires race"));
        RestartRace(); Race.Start();
        {
            const FVector F=(Track->Gates[9]-Track->Gates[8]).GetSafeNormal2D();
            FVector Start=(Track->Gates[8]+Track->Gates[9])*.5-F*5000;
            Start.Z=Track->HeightAt(Start.X,Start.Y)+HoverHeight;
            SetActorLocationAndRotation(Start,F.Rotation(),false,nullptr,ETeleportType::TeleportPhysics);
            FlightVelocity=F*5000.;
        }
        SmokeStage=6; SmokeDeadline=SmokeAge+8.; break;
    case 6:
        Throttle=0;
        {
            const FVector P=GetActorLocation();
            const float Clearance=P.Z-Track->HeightAt(P.X,P.Y);
            SmokePeakClearance=FMath::Max(SmokePeakClearance,Clearance);
            bSmokeDescended |= FlightVelocity.Z<-100.;
            if (SmokeAge<SmokeDeadline) return;
            UE_LOG(LogTemp,Display,TEXT("CANYON_JUMP peak=%.1f landed=%.1f hp=%.1f"),SmokePeakClearance,Clearance,Race.Health);
            SmokeAssert(SmokePeakClearance>1400 && bSmokeDescended,TEXT("ramp launch produces airborne arc"));
            SmokeAssert(FMath::Abs(Clearance-HoverHeight)<150 && Race.Health>0,TEXT("hover suspension catches landing"));
        }
        FFileHelper::SaveStringToFile(TEXT("PASS: terrain, hover, thrust, swept wall rebound, health, 5-second respawns, 3-ship retirement, ramp flight and landing"),*(FPaths::ProjectSavedDir()/TEXT("CanyonSmoke.txt")));
        SmokeStage=7; FPlatformMisc::RequestExit(false); break;
    }
}
