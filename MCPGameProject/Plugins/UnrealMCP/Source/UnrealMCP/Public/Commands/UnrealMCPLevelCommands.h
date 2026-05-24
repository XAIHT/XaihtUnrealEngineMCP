#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for level / world MCP commands:
 *   - open_level          : load a map by content path
 *   - save_current_level  : save the level currently being edited
 *   - save_all            : save all dirty map + content packages
 *   - new_level           : create a new blank level (optionally save it)
 *   - get_current_level   : return the current level's name, path and actor count
 */
class UNREALMCP_API FUnrealMCPLevelCommands
{
public:
    FUnrealMCPLevelCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleOpenLevel(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSaveCurrentLevel(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSaveAll(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleNewLevel(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetCurrentLevel(const TSharedPtr<FJsonObject>& Params);
};
