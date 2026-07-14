// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MCPGameProjectTarget : TargetRules
{
	public MCPGameProjectTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		// Match the UE 5.8 installed-engine build settings (V7). UE 5.8's binary engine is
		// built with V7 defaults (UnreachableCode/ReturnType/Dangling warning levels = Error);
		// a target that shares build products with UnrealEditor MUST match, or UBT rejects the
		// build with "modifies the values of properties ... not allowed" (the old 5.7 V6 value
		// did exactly that against a 5.8 engine). Include order bumped to 5.8 to match.
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("MCPGameProject");
	}
}
