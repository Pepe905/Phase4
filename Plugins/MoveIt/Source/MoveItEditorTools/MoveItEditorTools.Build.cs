// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

using UnrealBuildTool;

public class MoveItEditorTools : ModuleRules
{
	public MoveItEditorTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnforceIWYU = true;
        //bUseUnity = false;

        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "MoveIt",
				// ... add other public dependencies that you statically link with here ...
			}
            );
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "UMG",
                "Slate",
                "SlateCore",
                "Blutility",
                "Projects",
                "RenderCore",
                "AssetTools",
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
