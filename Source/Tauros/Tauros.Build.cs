// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Tauros : ModuleRules
{
	public Tauros(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		var CargoTarget = GetCargoTargetTriple();

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.Add("kernel32.lib");
			PublicSystemLibraries.Add("advapi32.lib");
			PublicSystemLibraries.Add("bcrypt.lib");
			PublicSystemLibraries.Add("ntdll.lib");
			PublicSystemLibraries.Add("userenv.lib");
			PublicSystemLibraries.Add("ws2_32.lib");
			PublicSystemLibraries.Add("msvcrt.lib");
		}

		#region cxx

		PublicIncludePaths.Add(
			Path.Combine(PluginDirectory, @$"target\{CargoTarget}\cxxbridge")
		);

		# endregion


		var CargoProfile = GetCargoProfile();

		var LibFileName = GetLibFileName();

		PublicAdditionalLibraries.Add(
			Path.Combine(PluginDirectory, @$"target\{CargoTarget}\{CargoProfile}\tauros.dll.lib")
		);

		PublicIncludePaths.Add(
			Path.Combine(PluginDirectory, $@"crates\cxx-async\cxx-async\include"));

		PublicDelayLoadDLLs.Add(
			Path.Combine(PluginDirectory, @$"target\{CargoTarget}\{CargoProfile}\tauros.dll")
		);


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

	private string GetCargoTargetTriple()
	{
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			return "x86_64-pc-windows-msvc";
		}

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			return "x86_64-unknown-linux-musl";
		}

		throw new BuildException($"Unsupported Unreal platform for Rust cargo build: {Target.Platform}");
	}

	private string GetLibFileName()
	{
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			if (Target.WindowsPlatform.Compiler.IsMSVC())
			{
				return "tauros.lib";
			}

			if (Target.WindowsPlatform.Compiler.IsClang())
			{
				// return "libdwebble.a";
				return "tauros.lib";
			}
		}

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			return "tauros.a"; // Linux uses .a for static libraries
		}

		throw new BuildException($"Unsupported Unreal platform for Rust cargo build: {Target.Platform}");
	}

	private string GetCargoProfile()
	{
		return Target.Configuration == UnrealTargetConfiguration.DebugGame ? "debug" : "release";
	}
}