// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JY : ModuleRules
{
	public JY(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// [디버깅] 이 모듈은 최적화를 끈다 — Development 빌드에서도 변수가 "최적화됨"으로 안 뜨고
		//   중단점에서 값을 정상적으로 볼 수 있게 한다. Shipping엔 영향 없음(그때는 별도 취급).
		OptimizeCode = CodeOptimization.Never;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"PhysicsCore",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"UMG",
			"Slate",
			"MotionWarping",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"ModularGameplay",
			"DeveloperSettings",
			"Niagara",
			"NavigationSystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PrivateDependencyModuleNames.Add("NetCore");
		PrivateDependencyModuleNames.Add("AnimGraphRuntime");
		SetupIrisSupport(Target);

		PublicIncludePaths.AddRange(new string[] {
			"JY"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
