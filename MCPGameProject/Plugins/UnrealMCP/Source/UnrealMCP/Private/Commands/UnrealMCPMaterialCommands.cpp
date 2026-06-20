#include "Commands/UnrealMCPMaterialCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "AssetToolsModule.h"
#include "Modules/ModuleManager.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialEditingLibrary.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/MeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Texture2D.h"

FUnrealMCPMaterialCommands::FUnrealMCPMaterialCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_material"))
    {
        return HandleCreateMaterial(Params);
    }
    else if (CommandType == TEXT("create_material_instance"))
    {
        return HandleCreateMaterialInstance(Params);
    }
    else if (CommandType == TEXT("set_material_parameter"))
    {
        return HandleSetMaterialParameter(Params);
    }
    else if (CommandType == TEXT("assign_material"))
    {
        return HandleAssignMaterial(Params);
    }
    else if (CommandType == TEXT("set_material_color"))
    {
        return HandleSetMaterialColor(Params);
    }
    else if (CommandType == TEXT("get_material_info"))
    {
        return HandleGetMaterialInfo(Params);
    }
    else if (CommandType == TEXT("assign_material_to_all_slots"))
    {
        return HandleAssignMaterialToAllSlots(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown material command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString Name, Path;
    if (!Params->TryGetStringField(TEXT("name"), Name) || !Params->TryGetStringField(TEXT("path"), Path))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' or 'path' parameter"));
    }

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();

    UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, Path, UMaterial::StaticClass(), Factory);
    if (!NewAsset)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create material '%s' at '%s'"), *Name, *Path));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("path"), NewAsset->GetPathName());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params)
{
    FString Name, Path, ParentPath;
    if (!Params->TryGetStringField(TEXT("name"), Name) ||
        !Params->TryGetStringField(TEXT("path"), Path) ||
        !Params->TryGetStringField(TEXT("parent_material"), ParentPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name