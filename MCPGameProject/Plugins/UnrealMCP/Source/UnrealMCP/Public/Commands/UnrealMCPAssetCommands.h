#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for asset-management MCP commands:
 *   - import_asset     : import an external file (FBX, texture, audio, ...) into the project
 *   - duplicate_asset  : copy an asset to a new path
 *   - rename_asset     : move/rename an asset
 *   - delete_asset     : delete an asset
 *   - save_asset       : save an asset to disk
 *   - create_folder    : create a content-browser directory
 */
class UNREALMCP_API FUnrealMCPAssetCommands
{
public:
    FUnrealMCPAssetCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleImportAsset(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDuplicateAsset(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleRenameAsset(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDeleteAsset(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSaveAsset(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateFolder(const TSharedPtr<FJsonObject>& Params);
};
