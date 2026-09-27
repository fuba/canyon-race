#pragma once
#include "CoreMinimal.h"

inline FTransform CanyonSeatCalibration(const FVector& HeadPosition,float HeadYaw,const FVector& EyeAnchor)
{
    const FRotator Rotation(0,-HeadYaw,0);
    return FTransform(Rotation,EyeAnchor-Rotation.RotateVector(HeadPosition));
}
