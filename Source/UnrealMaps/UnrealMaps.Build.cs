namespace UnrealBuildTool.Rules
{
	public class UnrealMaps : ModuleRules
	{
		public UnrealMaps(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicDependencyModuleNames.AddRange(new string[] {
                "Core",
                "SlateCore",
                "Slate"
			});

            PrivateDependencyModuleNames.AddRange(new string[] {
                "CoreUObject",
                "Engine",
                "HTTP",
                "ImageWrapper"
            });
		}
	}
}
