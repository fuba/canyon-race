using UnrealBuildTool;
public class CanyonRaceEditorTarget : TargetRules
{
    public CanyonRaceEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("CanyonRace");
    }
}
