using UnrealBuildTool;
public class CanyonRaceTarget : TargetRules
{
    public CanyonRaceTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("CanyonRace");
    }
}
