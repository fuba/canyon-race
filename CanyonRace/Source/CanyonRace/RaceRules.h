#pragma once
#include "CoreMinimal.h"

enum class ERacePhase : uint8 { Ready, Racing, Respawning, Finished, Retired };

// Pure rules shared by the pawn and automation tests; seconds use race time, not wall time.
struct FRaceRules
{
    static constexpr float MaxHealth = 100.f;
    static constexpr float RespawnDelay = 5.f;
    ERacePhase Phase = ERacePhase::Ready;
    float Health = MaxHealth;
    int32 Ships = 3;
    double Elapsed = 0.;
    float RespawnRemaining = 0.f;
    float DamageCooldown = 0.f;
    int32 LastGate = 0;

    void Start() { if (Phase == ERacePhase::Ready) Phase = ERacePhase::Racing; }
    bool Tick(float Dt)
    {
        Dt = FMath::Max(0.f, Dt);
        DamageCooldown = FMath::Max(0.f, DamageCooldown - Dt);
        if (Phase == ERacePhase::Racing || Phase == ERacePhase::Respawning) Elapsed += Dt;
        if (Phase != ERacePhase::Respawning) return false;
        RespawnRemaining = FMath::Max(0.f, RespawnRemaining - Dt);
        if (RespawnRemaining > KINDA_SMALL_NUMBER) return false;
        Phase = ERacePhase::Racing;
        Health = MaxHealth;
        DamageCooldown = 1.5f;
        return true;
    }
    void DestroyShip()
    {
        if (Phase != ERacePhase::Racing) return;
        Health = 0.f;
        --Ships;
        Phase = Ships > 0 ? ERacePhase::Respawning : ERacePhase::Retired;
        RespawnRemaining = Ships > 0 ? RespawnDelay : 0.f;
    }
    static float ImpactDamage(float InwardSpeedCm)
    {
        // Scrapes below 6 m/s are harmless; one ordinary impact cannot destroy a fresh ship.
        return FMath::Clamp((InwardSpeedCm - 600.f) * 0.012f, 0.f, 75.f);
    }
    bool Hit(float InwardSpeedCm)
    {
        if (Phase != ERacePhase::Racing || DamageCooldown > 0.f) return false;
        const float Damage = ImpactDamage(InwardSpeedCm);
        if (Damage <= 0.f) return false;
        Health = FMath::Max(0.f, Health - Damage);
        DamageCooldown = .4f;
        if (Health <= 0.f) DestroyShip();
        return true;
    }
    void PassGate(int32 Index, int32 Count)
    {
        if (Phase != ERacePhase::Racing || Index != LastGate + 1 || Index >= Count) return;
        LastGate = Index;
        if (LastGate == Count - 1) Phase = ERacePhase::Finished;
    }
};

inline bool CrossedRaceGate(const FVector& A, const FVector& B, const FVector& Gate, const FVector& Forward)
{
    const double D0 = FVector::DotProduct(A - Gate, Forward);
    const double D1 = FVector::DotProduct(B - Gate, Forward);
    if (D0 >= 0. || D1 < 0. || D1 - D0 < UE_SMALL_NUMBER) return false;
    const FVector At = FMath::Lerp(A, B, D0 / (D0 - D1)) - Gate;
    const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
    return FMath::Abs(FVector::DotProduct(At, Right)) <= 4500. && FMath::Abs(At.Z) <= 2500.;
}
