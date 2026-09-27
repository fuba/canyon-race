#include "HoverPawn.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"

void AHoverPawn::ContactTest()
{
    // Terrain contact fixtures from v0.2 stalls: nonzero velocity with unchanged position.
    const FVector Positions[]={FVector(-46025.592,13884.646,203.333),FVector(-75962.472,-35481.243,237.891),FVector(-65213.051,-28900.999,312.578),FVector(-48950.498,-17208.169,311.239)};
    const FVector Velocities[]={FVector(-3711.603,5429.196,-.016),FVector(4704.405,2622.747,52.632),FVector(7870.614,4281.733,51.515),FVector(4257.938,5514.808,28.919)};
    bool Passed=true;
    FString Report;
    for (int32 Legacy=0;Legacy<2;++Legacy) for (int32 Case=0;Case<4;++Case)
    {
        RestartRace();Race.Start();Race.DamageCooldown=10.f;
        SetActorLocationAndRotation(Positions[Case],FRotator(0,Velocities[Case].Rotation().Yaw,0),false,nullptr,ETeleportType::TeleportPhysics);
        FlightVelocity=Velocities[Case];Throttle=1.f;ContactHits=0;
        float Stalled=0.f,MaxStall=0.f;
        for (int32 Step=0;Step<240;++Step)
        {
            // Maintain downward pressure, as when pushing the nose into a floor while
            // moving tangentially. A one-hit sweep loses the entire step at the contact.
            FlightVelocity.Z=-600.;
            const FVector Before=GetActorLocation();
            if (!Legacy) MoveWithContacts(1.f/120.f);
            else
            {
                // Reproduce the former one-hit movement algorithm for a diagnostic comparison.
                FHitResult Hit;AddActorWorldOffset(FlightVelocity/120.f,true,&Hit);
                if (Hit.bBlockingHit)
                {
                    const double Inward=FMath::Max(0.,-FVector::DotProduct(FlightVelocity,Hit.Normal));
                    FlightVelocity+=Hit.Normal*Inward*1.4;
                    if (Hit.bStartPenetrating) AddActorWorldOffset(Hit.Normal*(Hit.PenetrationDepth+2.f),false);
                }
            }
            Stalled=FVector::Dist(Before,GetActorLocation())<.01f?Stalled+1.f/120.f:0.f;
            MaxStall=FMath::Max(MaxStall,Stalled);
        }
        const float Distance=FVector::Dist(Positions[Case],GetActorLocation());
        const bool OK=Distance>500.f && MaxStall<.25f;
        if (!Legacy) Passed &= OK;
        Report+=FString::Printf(TEXT("%s case=%d distance_cm=%.1f max_stall_s=%.3f contacts=%d %s\n"),Legacy?TEXT("legacy comparison"):TEXT("fixed"),Case,Distance,MaxStall,ContactHits,OK?TEXT("MOVING"):TEXT("STALLED"));
    }
    Report=(Passed?FString(TEXT("PASS: ")):FString(TEXT("FAIL: ")))+TEXT("terrain contact recovery\n")+Report;
    FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("CanyonContact.txt")));
    UE_LOG(LogTemp,Display,TEXT("%s"),*Report);
    FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);
}
