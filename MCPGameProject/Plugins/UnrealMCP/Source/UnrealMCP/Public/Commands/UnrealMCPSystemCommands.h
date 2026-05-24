#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for system-level MCP commands.
 *
 * These are "escape hatch" and introspection commands that let an AI client
 * reach engine functionality not yet covered by a dedicated command:
 *   - execute_python           : run a Python script inside the editor
 *   - execute_console_command  : run an editor console / CVar command
 *   - get_class_info           : reflect a UClass (parent, properties, functions)
 *   - list_assets              : enumerate assets under a content path
 */
class UNREALMCP_API FUnrealMCPSystemCommands
{
public:
    FUnrealMCPSystemCommands();

    // Handle system commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleExecutePython(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleExecuteConsoleCommand(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetClassInfo(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListAssets(const TSharedPtr<FJsonObject>& Params);
};
