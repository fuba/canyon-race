#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "CanyonGameMode.generated.h"

UCLASS()
class CANYONRACE_API ACanyonGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACanyonGameMode();
};

UCLASS()
class CANYONRACE_API ACanyonHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
