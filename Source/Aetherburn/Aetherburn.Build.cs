// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Aetherburn : ModuleRules
{
	public Aetherburn(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"InputCore",
			"EnhancedInput",
			"Niagara",
			"AnimGraphRuntime",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "AnimGraph", "BlueprintGraph", "UnrealEd" });
		}

		PublicIncludePaths.AddRange(new string[] {
			"Aetherburn",
			"Aetherburn/Variant_Horror",
			"Aetherburn/Variant_Horror/UI",
			"Aetherburn/Variant_Shooter",
			"Aetherburn/Variant_Shooter/AI",
			"Aetherburn/Variant_Shooter/UI",
			"Aetherburn/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
