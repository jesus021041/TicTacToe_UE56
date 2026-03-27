// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TicTacToe_UE56 : ModuleRules
{
    public TicTacToe_UE56(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Aggiunti "UMG", "Slate" e "SlateCore" per permettere l'uso dei Widget UI in C++
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore" });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}