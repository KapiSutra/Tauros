// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Tauros : ModuleRules
{
	public Tauros(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;


		PublicIncludePaths.AddRange(
			new string[]
			{
				// ... add public include paths required here ...
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
				// ... add another private include paths required here ...
			}
		);


		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
				"CoreOnline",
				"OnlineServicesInterface",
				"OnlineSubsystemSteam"
			}
		);

		// AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");


		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"EOSSDK",
				"EOSShared",
				"Steamworks",
				"HttpServer",
				"Dwebble",
				"HTTP",
				"Json",
				"UE5Coro"
				// ... add private dependencies that you statically link with here ...	
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}