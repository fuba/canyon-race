#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CanyonTrack.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

UCLASS()
class CANYONRACE_API ACanyonTrack : public AActor
{
    GENERATED_BODY()
public:
    ACanyonTrack();
    virtual void BeginPlay() override;
    bool EnsureBuilt();
    float HeightAt(double X, double Y) const;
    FVector GateForward(int32 Index) const;
    FTransform SpawnAt(int32 Gate) const;
    TArray<FVector> Gates;
    FString CourseName=TEXT("RIVER RUN"),RouteFile=TEXT("route.csv");
    static constexpr int32 GridSize = 505;
    static constexpr float Spacing = 800.f;
    static constexpr float Extent = 201600.f;
    UPROPERTY(EditAnywhere, Category="Canyon") TObjectPtr<UMaterialInterface> TerrainMaterial;
    UPROPERTY(EditAnywhere, Category="Canyon") TObjectPtr<UMaterialInterface> AccentMaterial;
private:
    TArray<float> Heights;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Chunks;
    bool bBuilt = false;
    void BuildGates();
};
