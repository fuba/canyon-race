#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "CanyonMovement.generated.h"

// Pawn owns the fixed-step integration; use UE's tested swept movement/depenetration.
UCLASS()
class UCanyonMovement : public UPawnMovementComponent
{
    GENERATED_BODY()
public:
    int32 PenetrationAttempts=0;
protected:
    virtual bool ResolvePenetrationImpl(const FVector& Adjustment,const FHitResult& Hit,const FQuat& Rotation) override
    {
        ++PenetrationAttempts;
        return Super::ResolvePenetrationImpl(Adjustment,Hit,Rotation);
    }
};
