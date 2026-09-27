#include "CanyonGameMode.h"
#include "HoverPawn.h"
#include "CanyonTrack.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "HeadMountedDisplayFunctionLibrary.h"

ACanyonGameMode::ACanyonGameMode()
{
    DefaultPawnClass=AHoverPawn::StaticClass();
    HUDClass=ACanyonHUD::StaticClass();
}

void ACanyonHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled()) return;
    const AHoverPawn* P=Cast<AHoverPawn>(GetOwningPawn());
    if (!P) return;
    const float W=Canvas->ClipX, H=Canvas->ClipY;
    DrawRect(FLinearColor(.015f,.025f,.035f,.85f),24,22,600,168);
    DrawText(TEXT("CANYON / ")+(P->Track?P->Track->CourseName:TEXT("TIME ATTACK")),FLinearColor(.2f,.9f,1),42,34,nullptr,1.5f);
    DrawText(FString::Printf(TEXT("%03.0f km/h     HULL %03.0f     SHIPS %d / 3"),P->FlightVelocity.Size()/100.*3.6,P->Race.Health,P->Race.Ships),FLinearColor::White,42,64,nullptr,1.4f);
    DrawText(FString::Printf(TEXT("TIME %.2f s     GATE %02d / %02d"),P->Race.Elapsed,P->Race.LastGate,P->Track?P->Track->Gates.Num()-1:0),FLinearColor(.65f,.75f,.8f),42,92,nullptr,1.2f);
    DrawText(P->StatusText(),FLinearColor(1,.65f,.2f),42,122,nullptr,1.1f);
    DrawText(P->RecordText(),FLinearColor(.2f,.9f,1),42,150,nullptr,1.15f);
    DrawRect(FLinearColor(.02f,.03f,.04f,.85f),24,H-66,W-48,42);
    DrawText(TEXT("W thrust   AD steer   DOWN/UP lift/dive   Q/E airbrakes   S brake   SHIFT boost   BACKSPACE recover   R restart   ESC quit"),FLinearColor(.75f,.85f,.9f),36,H-54,nullptr,.95f);
    if (P->Track && P->Track->Gates.IsValidIndex(P->Race.LastGate+1))
    {
        const FVector Goal=P->Track->Gates[P->Race.LastGate+1]+FVector(0,0,900);
        FVector2D Screen;
        if (GetOwningPlayerController()->ProjectWorldLocationToScreen(Goal,Screen) && Screen.X>20 && Screen.X<W-120 && Screen.Y>165 && Screen.Y<H-80)
            DrawText(FString::Printf(TEXT("NEXT %02d / %.0f m"),P->Race.LastGate+1,FVector::Dist(Goal,P->GetActorLocation())/100.),FLinearColor(.1f,1,1),Screen.X,Screen.Y,nullptr,1.15f);
    }
}
