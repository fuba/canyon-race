#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RaceRules.h"
#include "TimeTrial.h"
#include "HoverPawn.generated.h"

class USphereComponent;
class UCameraComponent;
class UTextRenderComponent;
class ACanyonTrack;
class UAudioComponent;
class USoundBase;
class UCanyonMovement;
class UProceduralMeshComponent;

UCLASS()
class CANYONRACE_API AHoverPawn : public APawn
{
    GENERATED_BODY()
public:
    AHoverPawn();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    FRaceRules Race;
    FVector FlightVelocity=FVector::ZeroVector;
    UPROPERTY(EditAnywhere, Category="Handling") float HoverHeight=350.f;
    UPROPERTY(EditAnywhere, Category="Handling") float Thrust=2400.f;
    UPROPERTY(EditAnywhere, Category="Handling") float TurnRate=65.f;
    UPROPERTY(EditAnywhere, Category="Handling") float MaxBankAngle=40.f;
    UPROPERTY(EditAnywhere, Category="Handling") float MaxPitchAngle=85.f;
    UPROPERTY(EditAnywhere, Category="Handling") float LateralGrip=.22f;
    UPROPERTY(EditAnywhere, Category="Handling") float MaxSpeed=11000.f;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Hull;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCanyonMovement> Movement;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> VernierFlames;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SeatTrackingRoot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Dashboard;
    UPROPERTY() TObjectPtr<ACanyonTrack> Track;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UAudioComponent> EngineAudio;
    UPROPERTY() TObjectPtr<USoundBase> ImpactSound;
    UPROPERTY() TObjectPtr<USoundBase> GateSound;
    FString StatusText() const;
    FString RecordText() const;
    FCanyonTimeTrial TimeTrial;
    void RestartRace();
private:
    const FVector SeatEye=FVector(105,0,155);
    bool bSeatCalibrated=false;
    bool bCalibrateOnFirstThrottle=true, bRuntimeRecenterPending=false;
    uint64 LastReferenceRevision=0;
    int32 SeatCalibrationCount=0;
    bool ApplySeatCalibration(const TCHAR* Reason);
    float TrackingStableTime=0.f;
    FString RecordFile;
    bool bSaveRecordFailed=false;
    void LoadRecord();
    void SaveRecord();
    FString TelemetryDir, TelemetryBuffer, FrameBuffer, InputBuffer;
    TArray<FKey> TelemetryKeys;
    TMap<FName,float> PreviousKeys;
    double TelemetryStart=0., PreviousFrameTime=0.;
    float TelemetryElapsed=0.f, TelemetryFlushElapsed=0.f;
    int64 TelemetryFrame=0;
    void StartTelemetry();
    void RecordTelemetry(float Dt);
    void FlushTelemetry();
    float Throttle=0.f, Steering=0.f, Brake=0.f, YawSpeed=0.f;
    float LiftInput=0.f, LeftAirBrake=0.f, RightAirBrake=0.f;
    float AppliedThrust=0.f, ReverseLeft=0.f, ReverseRight=0.f;
    float PitchAngle=0.f, PitchRate=0.f, RollRate=0.f;
    float BankAngle=0.f, BankAcceleration=0.f;
    int32 ContactHits=0;
    float RequestedMove=0.f, ActualMove=0.f, BlockedSeconds=0.f;
    FVector ContactNormal=FVector::ZeroVector;
    FString ContactName;
    void MoveWithContacts(float Dt);
    void UpdateVerniers();
    bool bFlightTest=false;
    bool bContactTest=false;
    void ContactTest();
    int32 FlightTestStage=0;
    double FlightTestAge=0.;
    FVector FlightTestStart;
    float FlightTestYaw=0.f;
    void FlightTestTick(float Dt);
    float FixedRemainder=0.f, DashElapsed=0.f;
    bool bBoost=false, bSmoke=false, bCaptured=false, bMuted=false, bCourseTest=false;
    double CourseAge=0.;
    TArray<float> FrameSamples;
    double BenchmarkStart=0., BenchmarkPrevious=0.;
    double SmokeAge=0.;
    int32 SmokeStage=0;
    double SmokeDeadline=0.;
    float SmokePeakClearance=0.f;
    bool bSmokeDescended=false;
    UPROPERTY() TObjectPtr<AActor> SmokeWall;
    void SetThrottle(float Value) { Throttle=FMath::Clamp(Value,0.f,1.f); }
    void SetSteering(float Value) { Steering=FMath::Clamp(Value,-1.f,1.f); }
    void SetBrake(float Value) { Brake=FMath::Clamp(Value,0.f,1.f); }
    void SetLift(float Value) { LiftInput=FMath::Clamp(Value,-1.f,1.f); }
    void SetLeftAirBrake(float Value) { LeftAirBrake=FMath::Clamp(Value,0.f,1.f); }
    void SetRightAirBrake(float Value) { RightAirBrake=FMath::Clamp(Value,0.f,1.f); }
    void BoostOn() { bBoost=true; }
    void BoostOff() { bBoost=false; }
    void Recover();
    void Recenter();
    void Quit();
    void ToggleSound();
    void Respawn();
    void MoveStep(float Dt);
    void UpdateDashboard();
    void SmokeTick(float Dt);
    void SmokeAssert(bool Condition, const TCHAR* Message);
    void CourseTick(float Dt);
    void BenchmarkTick();
};
