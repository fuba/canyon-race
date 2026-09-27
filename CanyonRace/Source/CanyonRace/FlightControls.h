#pragma once
#include "CoreMinimal.h"

// Arcade flight in cm/s and degrees. Bank assistance uses airflow; reverse jets do not.
struct FCanyonAerodynamics
{
    static float Authority(float ForwardSpeed)
    {
        return FMath::Square(FMath::Clamp(ForwardSpeed/3500.f,0.f,1.f));
    }
    static float ReverseThrust(float Left,float Right)
    {
        return 1800.f*(Left+Right);
    }
    static float ReverseYaw(float Left,float Right)
    {
        return 45.f*(Right-Left);
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
