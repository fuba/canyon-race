#include "CanyonTrack.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ACanyonTrack::ACanyonTrack()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    PrimaryActorTick.bCanEverTick = false;
}

void ACanyonTrack::BeginPlay()
{
    Super::BeginPlay();
    EnsureBuilt();
}

float ACanyonTrack::HeightAt(double X, double Y) const
{
    if (Heights.Num() != GridSize * GridSize) return 0.f;
    const double GX = FMath::Clamp((X + Extent) / Spacing, 0., double(GridSize - 1));
    const double GY = FMath::Clamp((Y + Extent) / Spacing, 0., double(GridSize - 1));
    const int32 IX = FMath::Min(FMath::FloorToInt(GX), GridSize - 2);
    const int32 IY = FMath::Min(FMath::FloorToInt(GY), GridSize - 2);
    return FMath::Lerp(FMath::Lerp(Heights[IY*GridSize+IX], Heights[IY*GridSize+IX+1], GX-IX),
        FMath::Lerp(Heights[(IY+1)*GridSize+IX], Heights[(IY+1)*GridSize+IX+1], GX-IX), GY-IY);
}

bool ACanyonTrack::EnsureBuilt()
{
    if (bBuilt) return true;
    TArray<uint8> Raw;
    const FString Folder = FPaths::ProjectContentDir() / TEXT("Canyon/Data");
    if (!FFileHelper::LoadFileToArray(Raw, *(Folder / TEXT("terrain.f32"))) || Raw.Num() != GridSize*GridSize*4)
    {
        UE_LOG(LogTemp, Error, TEXT("Canyon terrain data missing or malformed: %s"), *Folder);
        return false;
    }
    Heights.SetNumUninitialized(GridSize*GridSize);
    FMemory::Memcpy(Heights.GetData(), Raw.GetData(), Raw.Num());
    for (float H : Heights) if (!FMath::IsFinite(H) || H < -100.f || H > 100000.f) return false;
    TArray<FString> Lines;
    FString Course;
    if (FParse::Value(FCommandLine::Get(),TEXT("CanyonCourse="),Course) && Course==TEXT("summit"))
    {
        RouteFile=TEXT("route-summit.csv");CourseName=TEXT("SUMMIT DROP");
    }
    FFileHelper::LoadFileToStringArray(Lines, *(Folder / RouteFile));
    for (const FString& Line : Lines)
    {
        TArray<FString> Values; Line.ParseIntoArray(Values, TEXT(","));
        if (Values.Num() != 2 || !Values[0].IsNumeric() || !Values[1].IsNumeric()) continue;
        const double X = FCString::Atod(*Values[0]);
        const double Y = FCString::Atod(*Values[1]);
        Gates.Add(FVector(X,Y,HeightAt(X,Y)+350.f));
    }
    if (Gates.Num() < 2) return false;
    if (!TerrainMaterial) TerrainMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Canyon/Materials/M_Sandstone.M_Sandstone"));
    if (!AccentMaterial) AccentMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Canyon/Materials/M_Accent.M_Accent"));
    // Sixteen independently culled chunks, 508032 triangles. Keep collision synchronous at startup.
    constexpr int32 Quads = 126;
    for (int32 CY=0; CY<4; ++CY) for (int32 CX=0; CX<4; ++CX)
    {
        UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetCollisionProfileName(TEXT("BlockAll"));
        Mesh->bUseComplexAsSimpleCollision = true;
        Mesh->bUseAsyncCooking = false;
        Mesh->SetCastShadow(true);
        Mesh->RegisterComponent();
        Chunks.Add(Mesh);
        TArray<FVector> V,N;
        TArray<FVector2D> UV;
        TArray<FLinearColor> C;
        TArray<int32> Tri,WaterTri;
        TArray<FProcMeshTangent> Tan;
        for (int32 Y=0; Y<=Quads; ++Y) for (int32 X=0; X<=Quads; ++X)
        {
            const int32 IX=CX*Quads+X, IY=CY*Quads+Y;
            const double PX=IX*Spacing-Extent, PY=IY*Spacing-Extent;
            const float H=Heights[IY*GridSize+IX];
            const float DX=(HeightAt(PX+Spacing,PY)-HeightAt(PX-Spacing,PY))/(2.f*Spacing);
            const float DY=(HeightAt(PX,PY+Spacing)-HeightAt(PX,PY-Spacing))/(2.f*Spacing);
            const FVector Normal=FVector(-DX,-DY,1.).GetSafeNormal();
            V.Add(FVector(PX,PY,H)); N.Add(Normal); UV.Add(FVector2D(PX/4000.,PY/4000.));
            Tan.Add(FProcMeshTangent(FVector(1,0,DX).GetSafeNormal(),false));
            FLinearColor Color=FMath::Lerp(FLinearColor(.27f,.105f,.047f),FLinearColor(.55f,.31f,.13f),float(Normal.Z));
            C.Add(Color);
            if (X<Quads && Y<Quads)
            {
                const int32 I=Y*(Quads+1)+X;
                const float H1=HeightAt(PX+Spacing,PY),H2=HeightAt(PX,PY+Spacing),H3=HeightAt(PX+Spacing,PY+Spacing);
                // Separate the nearly level, low river samples from dry canyon rock.
                const bool River=FMath::Max(FMath::Max(H,H1),FMath::Max(H2,H3))<1900.f
                    && FMath::Max(FMath::Abs(H1-H),FMath::Abs(H2-H))<80.f;
                (River?WaterTri:Tri).Append({I,I+Quads+1,I+1,I+1,I+Quads+1,I+Quads+2});
            }
        }
        Mesh->CreateMeshSection_LinearColor(0,V,Tri,N,UV,C,Tan,true);
        Mesh->SetMaterial(0,TerrainMaterial);
        if (!WaterTri.IsEmpty())
        {
            Mesh->CreateMeshSection_LinearColor(1,V,WaterTri,N,UV,C,Tan,true);
            Mesh->SetMaterial(1,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Canyon/Materials/M_River.M_River")));
        }
    }
    // Instanced decorative bank rocks; the surveyed terrain remains the collision surface.
    FRandomStream Random(9272026);
    for (int32 Variant=0;Variant<3;++Variant)
    {
        UStaticMesh* Rock=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Canyon/Models/SM_Rock%d.SM_Rock%d"),Variant,Variant));
        if (!Rock) continue;
        UHierarchicalInstancedStaticMeshComponent* Stones=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        Stones->SetupAttachment(RootComponent);Stones->SetStaticMesh(Rock);
        Stones->SetCollisionProfileName(TEXT("NoCollision"));Stones->SetCullDistances(120000,160000);Stones->RegisterComponent();
        for (int32 I=0;I<600;++I)
        {
            const int32 Segment=Random.RandRange(0,Gates.Num()-2);
            FVector P=FMath::Lerp(Gates[Segment],Gates[Segment+1],Random.FRand());
            const FVector Side=FVector::CrossProduct(FVector::UpVector,(Gates[Segment+1]-Gates[Segment]).GetSafeNormal2D());
            P+=Side*Random.FRandRange(6000,30000)*(Random.RandRange(0,1)?1:-1);
            P.Z=HeightAt(P.X,P.Y);
            if (P.Z<1950 || P.Z>16000) continue;
            const float DX=(HeightAt(P.X+Spacing,P.Y)-HeightAt(P.X-Spacing,P.Y))/(2.f*Spacing);
            const float DY=(HeightAt(P.X,P.Y+Spacing)-HeightAt(P.X,P.Y-Spacing))/(2.f*Spacing);
            if (DX*DX+DY*DY>1.f) continue; // Rubble collects on banks, not halfway up sheer cliffs.
            const float Scale=Random.FRandRange(2.f,7.f);
            P.Z+=Scale*12.f;
            Stones->AddInstance(FTransform(FRotator(Random.FRandRange(-12,12),Random.FRandRange(0,360),Random.FRandRange(-10,10)),P,FVector(Scale,Scale,Scale)),true);
        }
    }
    BuildGates();
    // Two clearly artificial training ramps let us evaluate lift-off before designing a mountain route.
    for (int32 Segment : {8,16})
    {
        if (RouteFile!=TEXT("route.csv")) break;
        if (!Gates.IsValidIndex(Segment+1)) continue;
        FVector Center=(Gates[Segment]+Gates[Segment+1])*.5;
        Center.Z=HeightAt(Center.X,Center.Y)+15.f;
        const FVector Forward=(Gates[Segment+1]-Gates[Segment]).GetSafeNormal2D();
        const FVector Right=FVector::CrossProduct(FVector::UpVector,Forward);
        UProceduralMeshComponent* Ramp=NewObject<UProceduralMeshComponent>(this);
        Ramp->SetupAttachment(RootComponent); Ramp->SetCollisionProfileName(TEXT("BlockAll"));
        Ramp->bUseComplexAsSimpleCollision=true; Ramp->RegisterComponent(); Chunks.Add(Ramp);
        TArray<FVector> V={Center-Forward*4000-Right*1500,Center-Forward*4000+Right*1500,
            Center+Forward*4000-Right*1500+FVector(0,0,1200),Center+Forward*4000+Right*1500+FVector(0,0,1200)};
        TArray<int32> Indices={0,1,2,2,1,3};
        const FVector Normal=(FVector::UpVector-Forward*.15).GetSafeNormal();
        TArray<FVector> N={Normal,Normal,Normal,Normal};
        TArray<FVector2D> UV={FVector2D(0,0),FVector2D(0,1),FVector2D(1,0),FVector2D(1,1)};
        TArray<FLinearColor> Colors; TArray<FProcMeshTangent> Tangents;
        Ramp->CreateMeshSection_LinearColor(0,V,Indices,N,UV,Colors,Tangents,true);
        UMaterialInstanceDynamic* Mat=UMaterialInstanceDynamic::Create(AccentMaterial,this);
        if (Mat) Mat->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.035f,.085f,.12f));
        Ramp->SetMaterial(0,Mat);
    }
    bBuilt = true;
    UE_LOG(LogTemp, Display, TEXT("CANYON_TRACK_READY chunks=%d gates=%d"),Chunks.Num(),Gates.Num());
    return true;
}

FVector ACanyonTrack::GateForward(int32 Index) const
{
    if (Gates.Num()<2) return FVector::ForwardVector;
    Index=FMath::Clamp(Index,0,Gates.Num()-1);
    FVector Dir = Index==0 ? Gates[1]-Gates[0] : Gates[Index]-Gates[Index-1];
    Dir.Z=0.; return Dir.GetSafeNormal();
}

FTransform ACanyonTrack::SpawnAt(int32 Gate) const
{
    if (Gates.IsEmpty()) return FTransform(FVector(0,0,50000));
    Gate=FMath::Clamp(Gate,0,Gates.Num()-1);
    FVector Dir=Gate+1<Gates.Num() ? (Gates[Gate+1]-Gates[Gate]).GetSafeNormal2D() : GateForward(Gate);
    return FTransform(Dir.Rotation(),Gates[Gate]);
}

void ACanyonTrack::BuildGates()
{
    UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (int32 I=1; I<Gates.Num(); ++I)
    {
        const FVector F=GateForward(I), R=FVector::CrossProduct(FVector::UpVector,F);
        UMaterialInstanceDynamic* Mat=UMaterialInstanceDynamic::Create(AccentMaterial,this);
        if (Mat) Mat->SetVectorParameterValue(TEXT("Tint"),I==Gates.Num()-1?FLinearColor(1,.35f,.06f):FLinearColor(.02f,.8f,1));
        for (int32 Side : {-1,1})
        {
            UStaticMeshComponent* Pole=NewObject<UStaticMeshComponent>(this);
            Pole->SetupAttachment(RootComponent); Pole->SetStaticMesh(Cube);
            Pole->SetRelativeLocation(Gates[I]+R*(Side*4500.f)+FVector(0,0,700));
            Pole->SetRelativeScale3D(FVector(1.2,1.2,24));
            Pole->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Pole->SetMaterial(0,Mat); Pole->RegisterComponent();
        }
        UStaticMeshComponent* Bar=NewObject<UStaticMeshComponent>(this);
        Bar->SetupAttachment(RootComponent); Bar->SetStaticMesh(Cube);
        Bar->SetRelativeLocation(Gates[I]+FVector(0,0,1900));
        Bar->SetRelativeRotation(F.Rotation()); Bar->SetRelativeScale3D(FVector(.7,91,.7));
        Bar->SetCollisionEnabled(ECollisionEnabled::NoCollision); Bar->SetMaterial(0,Mat); Bar->RegisterComponent();
        UTextRenderComponent* Label=NewObject<UTextRenderComponent>(this);
        Label->SetupAttachment(RootComponent); Label->SetRelativeLocation(Gates[I]+FVector(0,0,2100));
        Label->SetRelativeRotation((-F).Rotation()); Label->SetWorldSize(400);
        Label->SetHorizontalAlignment(EHTA_Center);
        Label->SetText(FText::FromString(I==Gates.Num()-1?TEXT("FINISH"):FString::Printf(TEXT("%02d"),I)));
        Label->SetTextRenderColor(FColor(110,235,255)); Label->RegisterComponent();
    }
}
