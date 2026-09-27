#include "HoverPawn.h"
#include "CanyonTrack.h"
#include "HAL/PlatformProcess.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Crc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AHoverPawn::LoadRecord()
{
    // Automated drivers and capture teleports must never overwrite a player's best run.
    if (bSmoke || bCourseTest || bFlightTest || FParse::Param(FCommandLine::Get(),TEXT("CanyonCaptureShot")) || FString(FCommandLine::Get()).Contains(TEXT("CanyonCaptureGate="))) return;
    FString Route;FFileHelper::LoadFileToString(Route,*(FPaths::ProjectContentDir()/TEXT("Canyon/Data")/Track->RouteFile));
    Route.ReplaceInline(TEXT("\r\n"),TEXT("\n"));
    TArray<uint8> Terrain;FFileHelper::LoadFileToArray(Terrain,*(FPaths::ProjectContentDir()/TEXT("Canyon/Data/terrain.f32")));
    const uint32 CourseHash=FCrc::MemCrc32(Terrain.GetData(),Terrain.Num(),FCrc::StrCrc32(*Route));
    const FString Folder=FString(FPlatformProcess::UserSettingsDir())/TEXT("CanyonRace/Records");
    IFileManager::Get().MakeDirectory(*Folder,true);
    RecordFile=Folder/FString::Printf(TEXT("flight-v1-%08x.txt"),CourseHash);
    FString Text;
    if (FFileHelper::LoadFileToString(Text,*RecordFile))
    {
        const bool OK=TimeTrial.Load(Text,Track->Gates.Num()-1);
        UE_LOG(LogTemp,Display,TEXT("CANYON_RECORD_LOAD valid=%d course=%s"),OK,*Track->CourseName);
    }
}
void AHoverPawn::SaveRecord()
{
    if (RecordFile.IsEmpty()) return;
    const FString Temp=RecordFile+TEXT(".tmp");
    bSaveRecordFailed=!FFileHelper::SaveStringToFile(TimeTrial.Serialize(),*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
        || !IFileManager::Get().Move(*RecordFile,*Temp,true,true,false,true);
    UE_LOG(LogTemp,Display,TEXT("CANYON_RECORD_SAVE success=%d time=%.3f"),!bSaveRecordFailed,TimeTrial.Best.Last());
}
FString AHoverPawn::RecordText() const
{
    FString Text=TimeTrial.Best.IsEmpty()?TEXT("BEST --"):FString::Printf(TEXT("BEST %.2f s"),TimeTrial.Best.Last());
    if (TimeTrial.HasDelta()) Text+=FString::Printf(TEXT("  SPLIT %+.2f s"),TimeTrial.Delta());
    if (TimeTrial.bNewBest) Text+=TEXT("  NEW BEST");
    if (bSaveRecordFailed) Text+=TEXT("  SAVE FAILED");
    return Text;
}
