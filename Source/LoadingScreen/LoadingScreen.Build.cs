using UnrealBuildTool;

public class LoadingScreen : ModuleRules
{
    public LoadingScreen(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
            }
        );


        PrivateIncludePaths.AddRange(
            new string[] {
            }
        );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UMG",
                "MoviePlayer",
                "RenderCore",
                "PreLoadScreen",
                "DeveloperSettings"
            }
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "InputCore"
            }
        );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
            }
        );
    }
}
