#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Material command handlers for MCP
 * v2.0 - Added color convenience, multi-slot assignment, and material query
 */
class FUnrealMCPMaterialCommands
{
public:
    FUnrealMCPMaterialCommands();
    
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Core material commands
    TSharedPtr<FJsonObject> HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetMaterialParameter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAssignMaterial(const TSharedPtr<FJsonObject>& Params);
    
    // NEW: Convenience commands (Proposal #3)
    TSharedPtr<FJsonObject> HandleSetMaterialColor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetMaterialInfo(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAssignMaterialToAllSlots(const TSharedPtr<FJsonObject>& Params);
};