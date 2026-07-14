// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MCPGameProjectEditorTarget : TargetRules
{
	public MCPGameProjectEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		// UE 5.8 builds the installed engine with V7 defaults (UnreachableCode/ReturnType/Dangling
		// warning levels = Error). The editor target shares build products with UnrealEditor, so it
		// MUST match V7 or UBT rejects the build ("modifies the values of properties ... not allowed").
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("MCPGameProject");
	}
}
