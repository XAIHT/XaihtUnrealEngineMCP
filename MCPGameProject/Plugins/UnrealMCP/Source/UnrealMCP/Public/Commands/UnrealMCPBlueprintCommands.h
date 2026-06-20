#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Blueprint command handlers for MCP
 * v2.0 - Added save_blueprint, is_blueprint_dirty, and auto-save support
 */
class FUnrealMCPBlueprintCommands
{
public:
    FUnrealMCPBlueprintCommands();
    
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Core blueprint commands
    TSharedPtr<FJsonObject> HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetComponentProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetPhysicsProperties(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetBlueprintProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetStaticMeshProperties(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetPawnProperties(const TSharedPtr<FJsonObject>& Params);
    
    // NEW: Persistence commands (Proposal #5)
    TSharedPtr<FJsonObject> HandleSaveBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleIsBlueprintDirty(const TSharedPtr<FJsonObject>& Params);
};