// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RunBoyRun : ModuleRules
{
	public RunBoyRun(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
            "NavigationSystem",
            "Niagara",
            "MotionWarping",
            "DaySequence"
        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"RunBoyRun",
			"RunBoyRun/Variant_Platforming",
			"RunBoyRun/Variant_Platforming/Animation",
			"RunBoyRun/Variant_Combat",
			"RunBoyRun/Variant_Combat/AI",
			"RunBoyRun/Variant_Combat/Animation",
			"RunBoyRun/Variant_Combat/Gameplay",
			"RunBoyRun/Variant_Combat/Interfaces",
			"RunBoyRun/Variant_Combat/UI",
			"RunBoyRun/Variant_SideScrolling",
			"RunBoyRun/Variant_SideScrolling/AI",
			"RunBoyRun/Variant_SideScrolling/Gameplay",
			"RunBoyRun/Variant_SideScrolling/Interfaces",
			"RunBoyRun/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
