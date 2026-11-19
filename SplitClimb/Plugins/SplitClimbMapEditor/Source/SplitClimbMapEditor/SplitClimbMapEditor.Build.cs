// SplitClimb マップエディタ用プラグインモジュールのビルド設定。
// ゲームモジュール（SplitClimb）には依存せず、エディタ専用モジュールとして構成する。

using UnrealBuildTool;

public class SplitClimbMapEditor : ModuleRules
{
	public SplitClimbMapEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"EditorFramework",
			"PlacementMode",
			"ToolMenus",
			"Slate",
			"SlateCore"
		});
	}
}
