#include "RaceRules.h"
#include "Misc/AutomationTest.h"
#include "SeatCalibration.h"
#include "CanyonXRRecenter.h"
#include "FlightControls.h"
#include "TimeTrial.h"
#include "InputCoreTypes.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFrameKeysTest,"Canyon.Rules.NativeFrameKeysReachOpenXR",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFrameKeysTest::RunTest(const FString&)
{
    for (const TCHAR* Name:{TEXT("SteamFrame_Left_Shoulder_Click"),TEXT("SteamFrame_Right_Shoulder_Click"),TEXT("SteamFrame_Left_Thumbstick_Y"),TEXT("SteamFrame_Right_Trigger_Axis")})
    {
        const FKey Key(Name);
        TestTrue(Name,Key.IsValid());
        TestFalse(TEXT("Legacy OpenXR must not filter registered XR keys out as Blueprint-bindable"),Key.IsBindableInBlueprints());
    }
    TestFalse(TEXT("Frame left controller has no A button path"),FKey(TEXT("SteamFrame_Left_A_Click")).IsValid());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAeroTest,"Canyon.Rules.AerodynamicControls",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAeroTest::RunTest(const FString&)
{
    TestEqual(TEXT("No lift at zero airflow"),FCanyonAerodynamics::Lift(1,0),0.f);
    TestEqual(TEXT("Reversing does not create forward wing lift"),FCanyonAerodynamics::Lift(1,-5000),0.f);
    TestTrue(TEXT("Takeoff authority overcomes gravity at speed"),FCanyonAerodynamics::Lift(1,3500)>980);
    TestTrue(TEXT("Push forward forces descent"),FCanyonAerodynamics::Lift(-1,3500)<0);
    TestTrue(TEXT("Lift grows with airspeed"),FCanyonAerodynamics::Lift(1,1500)<FCanyonAerodynamics::Lift(1,3000));
    TestEqual(TEXT("Symmetric airbrakes do not steer"),FCanyonAerodynamics::BrakeYaw(1,1,5000),0.f);
    TestTrue(TEXT("Independent left and right brakes turn toward their own side"),FCanyonAerodynamics::BrakeYaw(1,0,5000)<0 && FCanyonAerodynamics::BrakeYaw(0,1,5000)>0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeTrialTest,"Canyon.Rules.PersistentBestAndSplits",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTimeTrialTest::RunTest(const FString&)
{
    FCanyonTimeTrial T;T.StartRun();
    TestFalse(TEXT("Cannot skip gates"),T.Gate(2,3,20));
    T.Gate(1,3,10);T.Gate(2,3,20);
    TestTrue(TEXT("Incomplete or DNF run never replaces best"),T.Best.IsEmpty());
    TestTrue(TEXT("First ordered finish records best"),T.Gate(3,3,30));
    T.StartRun();T.Gate(1,3,9);
    TestEqual(TEXT("Faster split is negative"),T.Delta(),-1.);
    T.Gate(2,3,19);TestTrue(TEXT("Improved finish replaces best"),T.Gate(3,3,28));
    TestEqual(TEXT("Finish comparison still uses previous best"),T.Delta(),-2.);
    FCanyonTimeTrial Loaded;
    TestTrue(TEXT("Saved record reloads"),Loaded.Load(T.Serialize(),3));
    TestFalse(TEXT("Wrong course gate count rejected"),Loaded.Load(T.Serialize(),4));
    TestFalse(TEXT("Nonmonotonic record rejected"),Loaded.Load(TEXT("10\n9\n30\n"),3));
    Loaded.StartRun();Loaded.Gate(1,3,12);Loaded.Gate(2,3,22);
    TestFalse(TEXT("Slower finish preserves best"),Loaded.Gate(3,3,35));
    TestEqual(TEXT("Original record survives"),Loaded.Best.Last(),28.);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReferenceRecenterTest,"Canyon.Rules.RuntimeRecenterTiming",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FReferenceRecenterTest::RunTest(const FString&)
{
    FCanyonReferenceChanges Changes;
    Changes.Queue(200);Changes.Queue(200);Changes.Queue(300);
    TestFalse(TEXT("Pending event must not calibrate against the old pose"),Changes.Advance(199));
    TestTrue(TEXT("Applies exactly on the first new-space frame"),Changes.Advance(200));
    TestEqual(TEXT("Duplicate events coalesce"),Changes.Revision,uint64(1));
    TestFalse(TEXT("No repeated alignment during normal head turns"),Changes.Advance(250));
    TestTrue(TEXT("A second Steam recenter is handled"),Changes.Advance(300));
    const FVector Eye(105,0,155);
    FTransform Seat=CanyonSeatCalibration(FVector(4,-26,-36),159,Eye);
    for (const FVector& NewHead:{FVector(0,0,0),FVector(30,-70,165)})
    {
        // Each new origin replaces the old calibration instead of multiplying another correction.
        Seat=CanyonSeatCalibration(NewHead,-20,Eye);
        TestTrue(TEXT("Repeated reset preserves pilot position"),Seat.TransformPosition(NewHead).Equals(Eye,.001));
        TestTrue(TEXT("Repeated reset preserves forward"),Seat.TransformVector(FRotator(0,-20,0).Vector()).Equals(FVector::ForwardVector,.001));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeatTest,"Canyon.Rules.SeatRecenter",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSeatTest::RunTest(const FString&)
{
    const FVector Anchor(105,0,155),Head(70,-210,168);
    for (float Yaw:{0.f,90.f,180.f,-135.f})
    {
        const FTransform Seat=CanyonSeatCalibration(Head,Yaw,Anchor);
        TestTrue(TEXT("Tracked head lands exactly at pilot eye"),Seat.TransformPosition(Head).Equals(Anchor,.001));
        TestTrue(TEXT("User forward aligns with craft forward including 180-degree room yaw"),Seat.TransformVector(FRotator(0,Yaw,0).Vector()).Equals(FVector::ForwardVector,.001));
        TestTrue(TEXT("Head motion remains one-to-one"),FMath::IsNearlyEqual((Seat.TransformPosition(Head+FVector(0,10,0))-Anchor).Size(),10.,.001));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRaceLifecycleTest,"Canyon.Rules.ThreeShipsAndFiveSeconds",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRaceLifecycleTest::RunTest(const FString&)
{
    FRaceRules R;
    R.Tick(12.f); TestEqual(TEXT("Ready time is not counted"),R.Elapsed,0.);
    R.Start(); R.Tick(2.f);
    for (int32 Ships=2; Ships>=1; --Ships)
    {
        R.DestroyShip(); R.DestroyShip();
        TestEqual(TEXT("Duplicate destruction does not consume another ship"),R.Ships,Ships);
        TestFalse(TEXT("Not before 5 seconds"),R.Tick(4.99f));
        TestTrue(TEXT("Respawn at 5 seconds"),R.Tick(.01f));
        TestEqual(TEXT("Full health on new ship"),R.Health,100.f);
    }
    TestTrue(TEXT("Race timer includes waiting"),FMath::IsNearlyEqual(R.Elapsed,12.,.001));
    R.DestroyShip(); TestEqual(TEXT("Zero ships after third loss"),R.Ships,0);
    TestTrue(TEXT("DNF after third loss"),R.Phase==ERacePhase::Retired);
    TestFalse(TEXT("No fourth ship"),R.Tick(100.f));
    TestTrue(TEXT("DNF time stops"),FMath::IsNearlyEqual(R.Elapsed,12.,.001));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRaceImpactTest,"Canyon.Rules.ImpactAndContactCooldown",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRaceImpactTest::RunTest(const FString&)
{
    FRaceRules R; R.Start();
    TestFalse(TEXT("Gentle scrape does no damage"),R.Hit(500));
    TestTrue(TEXT("Fast impact damages"),R.Hit(10000));
    TestEqual(TEXT("Fresh craft survives one collision"),R.Health,25.f);
    TestFalse(TEXT("Same contact does not duplicate damage"),R.Hit(10000));
    TestEqual(TEXT("Still alive during cooldown"),R.Ships,3);
    R.Tick(.41f); R.Hit(10000);
    TestTrue(TEXT("Repeated large impacts destroy craft"),R.Phase==ERacePhase::Respawning);
    TestFalse(TEXT("No damage while destroyed"),R.Hit(10000));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRaceGateTest,"Canyon.Rules.GateOrderAndSweptCrossing",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRaceGateTest::RunTest(const FString&)
{
    const FVector O=FVector::ZeroVector,F=FVector::ForwardVector;
    TestTrue(TEXT("High speed segment crossing counts"),CrossedRaceGate(FVector(-10000,0,0),FVector(10000,0,0),O,F));
    TestFalse(TEXT("Wrong direction does not count"),CrossedRaceGate(FVector(100,0,0),FVector(-100,0,0),O,F));
    TestFalse(TEXT("Outside width does not count"),CrossedRaceGate(FVector(-100,4600,0),FVector(100,4600,0),O,F));
    TestFalse(TEXT("Above gate does not count"),CrossedRaceGate(FVector(-100,0,2600),FVector(100,0,2600),O,F));
    FRaceRules R; R.Start(); R.PassGate(2,4);
    TestEqual(TEXT("Cannot skip checkpoint"),R.LastGate,0);
    R.PassGate(1,4); R.DestroyShip(); R.Tick(5);
    TestEqual(TEXT("Checkpoint survives destruction"),R.LastGate,1);
    R.PassGate(2,4); R.PassGate(3,4);
    TestTrue(TEXT("Final ordered gate finishes"),R.Phase==ERacePhase::Finished);
    const double FinishTime=R.Elapsed; R.Tick(10);
    TestEqual(TEXT("Finish time frozen"),R.Elapsed,FinishTime);
    return true;
}
#endif
