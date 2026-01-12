// copyright yarn spinner pty ltd
// licensed under the mit license. see license.md in project root

using System.IO;
using UnrealBuildTool;

public class YarnSpinnerEditor : ModuleRules
{
	public YarnSpinnerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"YarnSpinner"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"AssetTools",
			"Projects",
			"Json",
			"JsonUtilities"
		});

		// define ysc compiler path for use in code
		string PluginPath = Path.Combine(ModuleDirectory, "../../");
		string YscPath;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			YscPath = Path.Combine(PluginPath, "Tools/Win64/ysc.exe");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			YscPath = Path.Combine(PluginPath, "Tools/Mac/ysc");
		}
		else
		{
			YscPath = "ysc"; // fallback to system path
		}

		// make ysc path available in code via define
		PrivateDefinitions.Add(string.Format("YSC_PATH=TEXT(\"{0}\")", YscPath.Replace("\\", "/")));
	}
}
