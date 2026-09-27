#include "HoverPawn.h"
#include "ProceduralMeshComponent.h"

void AHoverPawn::UpdateVerniers()
{
    const bool Flying=Race.Phase==ERacePhase::Ready || Race.Phase==ERacePhase::Racing;
    VernierFlames->SetVisibility(Flying);
    if (!Flying) return;
    struct FJet { FVector Position,Exhaust;float Power,Length; };
    TArray<FJet,TInlineAllocator<18>> Jets;
    auto Add=[&](FVector P,FVector D,float Power,float Length=22.f){Jets.Add({P,D,FMath::Clamp(Power,0.f,1.f),Length});};
    for (float Side:{-1.f,1.f})
    {
        Add(FVector(-222,Side*177,-25),FVector(-1,0,0),Throttle,38);
        Add(FVector(340,Side*177,-15),FVector(1,0,0),Side<0?ReverseLeft:ReverseRight,30);
        for (float X:{-110.f,200.f}) Add(FVector(X,Side*177,-69),FVector(0,0,-1),.28f,16);
    }
    const float Up=FMath::Max(0.f,PitchRate/100.f),Down=FMath::Max(0.f,-PitchRate/100.f);
    Add(FVector(350,0,-36),FVector(0,0,-1),Up);
    Add(FVector(-150,0,0),FVector(0,0,1),Up);
    Add(FVector(350,0,12),FVector(0,0,1),Down);
    Add(FVector(-150,0,-54),FVector(0,0,-1),Down);
    const float RollRight=FMath::Max(0.f,RollRate/90.f),RollLeft=FMath::Max(0.f,-RollRate/90.f);
    Add(FVector(60,210,12),FVector(0,0,1),RollRight);
    Add(FVector(60,-210,-65),FVector(0,0,-1),RollRight);
    Add(FVector(60,-210,12),FVector(0,0,1),RollLeft);
    Add(FVector(60,210,-65),FVector(0,0,-1),RollLeft);
    Add(FVector(60,-223,-25),FVector(0,-1,0),FMath::Max(0.f,BankAcceleration/2700.f));
    Add(FVector(60,223,-25),FVector(0,1,0),FMath::Max(0.f,-BankAcceleration/2700.f));
    TArray<FVector> V,N;
    TArray<FVector2D> UV;
    TArray<FLinearColor> Colors;
    TArray<int32> Indices;
    TArray<FProcMeshTangent> Tangents;
    // Eighteen tiny eight-sided plumes in one section: no particles, lights, shadows or collision.
    for (int32 J=0;J<Jets.Num();++J)
    {
        const FJet& Jet=Jets[J];FVector U,W;Jet.Exhaust.FindBestAxisVectors(U,W);
        const int32 Base=V.Num();
        const float Pulse=.95f+.05f*FMath::Sin(GetGameTimeSinceCreation()*24.f+J*1.7f);
        for (int32 I=0;I<8;++I)
        {
            const float Angle=I*2.f*PI/8.f;
            V.Add(Jet.Position+(U*FMath::Cos(Angle)+W*FMath::Sin(Angle))*3.5f);
            N.Add(-Jet.Exhaust);UV.Add(FVector2D::ZeroVector);
            Colors.Add(FLinearColor(1.3f,1.3f,1.3f,Jet.Power*.65f));
            Indices.Append({Base+I,Base+(I+1)%8,Base+8});
        }
        V.Add(Jet.Position+Jet.Exhaust*Jet.Length*Jet.Power*Pulse);
        N.Add(-Jet.Exhaust);UV.Add(FVector2D::ZeroVector);Colors.Add(FLinearColor(1.3f,1.3f,1.3f,0));
    }
    if (!VernierFlames->GetProcMeshSection(0)) VernierFlames->CreateMeshSection_LinearColor(0,V,Indices,N,UV,Colors,Tangents,false);
    else VernierFlames->UpdateMeshSection_LinearColor(0,V,N,UV,Colors,Tangents);
}
