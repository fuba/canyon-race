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
};
