namespace UnrealBuildTool.Rules
{
	public class UnrealMaps : ModuleRules
	{
		public UnrealMaps(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(new string[] {
                "Core",
				"Slate"
			});
		}
	}
}
