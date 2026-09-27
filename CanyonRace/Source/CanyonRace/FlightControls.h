#pragma once
#include "CoreMinimal.h"

// Arcade aerodynamics in cm/s. Control authority comes from forward airflow, not altitude.
struct FCanyonAerodynamics
{
    static float Authority(float ForwardSpeed)
    {
        return FMath::Square(FMath::Clamp(ForwardSpeed/3500.f,0.f,1.f));
    }
    static float Lift(float PullBack,float ForwardSpeed)
    {
        return 1800.f*FMath::Clamp(PullBack,-1.f,1.f)*Authority(ForwardSpeed);
    }
    static float BrakeYaw(float Left,float Right,float ForwardSpeed)
    {
        return 45.f*(Right-Left)*Authority(ForwardSpeed);
    }
    static float BankAcceleration(float BankDegrees,float ForwardSpeed)
    {
        return 4200.f*FMath::Sin(FMath::DegreesToRadians(BankDegrees))*Authority(ForwardSpeed);
    }
    static float BankYaw(float SideAcceleration,float ForwardSpeed)
    {
        // Coordinated turn: faster flight needs a wider arc for the same bank.
        return FMath::RadiansToDegrees(SideAcceleration/FMath::Max(ForwardSpeed,2000.f));
    }
};
