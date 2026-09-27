#include "HoverPawn.h"
#include "CanyonMovement.h"
#include "Kismet/GameplayStatics.h"

void AHoverPawn::MoveWithContacts(float Dt)
{
    const FVector Before=GetActorLocation();
    RequestedMove+=FlightVelocity.Size()*Dt;
    float Remaining=Dt;
    // Consume the remaining time after an impact instead of throwing away all tangential
    // motion on a near-zero-time ground hit. SafeMove resolves overlapping contact shapes.
    for (int32 Pass=0;Pass<3 && Remaining>UE_SMALL_NUMBER;++Pass)
    {
        FHitResult Hit;
        Movement->SafeMoveUpdatedComponent(FlightVelocity*Remaining,GetActorQuat(),true,Hit);
        if (!Hit.bBlockingHit) break;
        ++ContactHits;ContactNormal=Hit.Normal;ContactName=GetNameSafe(Hit.GetActor());
        const float Inward=FMath::Max(0.f,float(-FVector::DotProduct(FlightVelocity,Hit.Normal)));
        if (Inward>0)
        {
            // Gentle floor contacts slide; harder impacts retain the existing rebound.
            FlightVelocity+=Hit.Normal*Inward*(Inward>600.f?1.4f:1.f);
            if (Race.Hit(Inward))
            {
                UE_LOG(LogTemp,Display,TEXT("CANYON_IMPACT speed=%.1f hp=%.1f actor=%s"),Inward,Race.Health,*ContactName);
                if (!bMuted && ImpactSound) UGameplayStatics::PlaySound2D(this,ImpactSound,.65f);
            }
        }
        if (Race.Phase==ERacePhase::Respawning || Race.Phase==ERacePhase::Retired) break;
        Remaining*=1.f-Hit.Time;
        if (Hit.Time<.001f)
        {
            Movement->SlideAlongSurface(FlightVelocity*Remaining,1.f,Hit.Normal,Hit,false);
            break;
        }
    }
    const float Distance=FVector::Dist(Before,GetActorLocation());
    ActualMove+=Distance;
    if (Distance<.01f && FlightVelocity.Size()>100.f)
    {
        BlockedSeconds+=Dt;
        if (BlockedSeconds-Dt<.25f && BlockedSeconds>=.25f)
            UE_LOG(LogTemp,Warning,TEXT("CANYON_BLOCKED pos=%s velocity=%s contact=%s normal=%s"),*GetActorLocation().ToString(),*FlightVelocity.ToString(),*ContactName,*ContactNormal.ToString());
    }
    else BlockedSeconds=0.f;
}
