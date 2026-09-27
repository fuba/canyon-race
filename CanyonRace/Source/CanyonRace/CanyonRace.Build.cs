using UnrealBuildTool;
public class CanyonRace : ModuleRules
{
    public CanyonRace(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        string Version = System.IO.File.ReadAllText(System.IO.Path.Combine(ModuleDirectory, "../../../VERSION")).Trim();
        PublicDefinitions.Add("CANYON_VERSION=\"" + Version + "\"");
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "XRBase", "ProceduralMeshComponent", "RHI", "RenderCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "OpenXRHMD", "AugmentedReality" });
        AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenXR");
    }
}
