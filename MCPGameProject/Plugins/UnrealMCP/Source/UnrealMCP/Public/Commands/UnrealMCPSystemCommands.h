#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * System command handlers for MCP
 * v2.0 - Added execute_python_file, exception tracebacks, and robust error reporting
 */
class FUnrealMCPSystemCommands
{
public:
    FUnrealMCPSystemCommands();
    
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Core system commands
    TSharedPtr<FJsonObject> HandleExecutePython(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleExecuteConsoleCommand(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetClassInfo(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListAssets(const TSharedPtr<FJsonObject>& Params);
    
    // NEW: Execute Python from file (Proposal #6)
    TSharedPtr<FJsonObject> HandleExecutePythonFile(const TSharedPtr<FJsonObject>& Params);
};