// copyright yarn spinner pty ltd
// licensed under the mit license. see license.md in project root

using System.IO;
using UnrealBuildTool;

public class YarnSpinner : ModuleRules
{
	public YarnSpinner(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// basic ue dependencies - no protobuf needed!
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});

		// add nanopb include paths
		// ModuleDirectory is Source/YarnSpinner/, so ../ThirdParty gets us to Source/ThirdParty
		string ThirdPartyPath = Path.Combine(ModuleDirectory, "../ThirdParty");
		string NanopbPath = Path.Combine(ThirdPartyPath, "nanopb");
		string GeneratedPath = Path.Combine(ModuleDirectory, "Generated");

		PublicIncludePaths.Add(NanopbPath);
		PublicIncludePaths.Add(GeneratedPath);

		// add nanopb source files to the build
		// these compile directly with our module - no external library needed
		PrivateDefinitions.Add("PB_FIELD_32BIT=1"); // use 32-bit field numbers

		// note: UBT will automatically find and compile .c files in our source directories
		// we just need to make sure the include paths are set up correctly (done above)
	}
}
