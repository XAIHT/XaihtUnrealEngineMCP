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
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name', 'path', or 'parent_material' parameter"));
    }

    UMaterialInterface* ParentMaterial = LoadObject<UMaterialInterface>(nullptr, *ParentPath);
    if (!ParentMaterial)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Parent material not found: %s"), *ParentPath));
    }

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
    Factory->InitialParent = ParentMaterial;

    UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, Path, UMaterialInstanceConstant::StaticClass(), Factory);
    if (!NewAsset)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create material instance '%s' at '%s'"), *Name, *Path));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("path"), NewAsset->GetPathName());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleSetMaterialParameter(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath, ParameterName;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath) || !Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' or 'parameter_name' parameter"));
    }

    UMaterialInstanceConstant* MaterialInstance = LoadObject<UMaterialInstanceConstant>(nullptr, *MaterialPath);
    if (!MaterialInstance)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material instance not found: %s"), *MaterialPath));
    }

    double NumberValue;
    if (Params->TryGetNumberField(TEXT("value"), NumberValue))
    {
        UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MaterialInstance, FName(*ParameterName), static_cast<float>(NumberValue));
        return FUnrealMCPCommonUtils::CreateSuccessResponse();
    }

    const TArray<TSharedPtr<FJsonValue>>* ColorArray;
    if (Params->TryGetArrayField(TEXT("value"), ColorArray) && ColorArray->Num() >= 3)
    {
        const float A = ColorArray->Num() > 3 ? static_cast<float>((*ColorArray)[3]->AsNumber()) : 1.0f;
        const FLinearColor Color(static_cast<float>((*ColorArray)[0]->AsNumber()), static_cast<float>((*ColorArray)[1]->AsNumber()), static_cast<float>((*ColorArray)[2]->AsNumber()), A);
        UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(MaterialInstance, FName(*ParameterName), Color);
        return FUnrealMCPCommonUtils::CreateSuccessResponse();
    }

    FString TexturePath;
    if (Params->TryGetStringField(TEXT("value"), TexturePath))
    {
        UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
        if (!Texture)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Texture not found: %s"), *TexturePath));
        }

        UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MaterialInstance, FName(*ParameterName), Texture);
        return FUnrealMCPCommonUtils::CreateSuccessResponse();
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing or unsupported 'value' parameter"));
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleAssignMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName, MaterialPath;
    int32 SlotIndex = 0;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName) || !Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' or 'material_path' parameter"));
    }
    Params->TryGetNumberField(TEXT("slot_index"), SlotIndex);

    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Editor world not available"));
    }

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Actors);
    for (AActor* Actor : Actors)
    {
        const bool bNameMatches = Actor && (Actor->GetName() == ActorName
#if WITH_EDITOR
            || Actor->GetActorLabel() == ActorName
#endif
        );

        if (bNameMatches)
        {
            TArray<UMeshComponent*> MeshComponents;
            Actor->GetComponents<UMeshComponent>(MeshComponents);
            for (UMeshComponent* MeshComponent : MeshComponents)
            {
                MeshComponent->SetMaterial(SlotIndex, Material);
            }
            return FUnrealMCPCommonUtils::CreateSuccessResponse();
        }
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleSetMaterialColor(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath, ParameterName;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath) || !Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' or 'parameter_name' parameter"));
    }

    UMaterialInstanceConstant* MaterialInstance = LoadObject<UMaterialInstanceConstant>(nullptr, *MaterialPath);
    if (!MaterialInstance)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material instance not found: %s"), *MaterialPath));
    }

    const TArray<TSharedPtr<FJsonValue>>* ColorArray;
    if (!Params->TryGetArrayField(TEXT("color"), ColorArray) || ColorArray->Num() < 3)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'color' array parameter"));
    }

    const float A = ColorArray->Num() > 3 ? static_cast<float>((*ColorArray)[3]->AsNumber()) : 1.0f;
    const FLinearColor Color(static_cast<float>((*ColorArray)[0]->AsNumber()), static_cast<float>((*ColorArray)[1]->AsNumber()), static_cast<float>((*ColorArray)[2]->AsNumber()), A);
    UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(MaterialInstance, FName(*ParameterName), Color);
    return FUnrealMCPCommonUtils::CreateSuccessResponse();
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleGetMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), Material->GetName());
    ResultObj->SetStringField(TEXT("path"), Material->GetPathName());
    ResultObj->SetStringField(TEXT("class"), Material->GetClass()->GetName());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleAssignMaterialToAllSlots(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName, MaterialPath;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName) || !Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' or 'material_path' parameter"));
    }

    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Editor world not available"));
    }

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Actors);
    for (AActor* Actor : Actors)
    {
        const bool bNameMatches = Actor && (Actor->GetName() == ActorName
#if WITH_EDITOR
            || Actor->GetActorLabel() == ActorName
#endif
        );

        if (bNameMatches)
        {
            TArray<UMeshComponent*> MeshComponents;
            Actor->GetComponents<UMeshComponent>(MeshComponents);
            for (UMeshComponent* MeshComponent : MeshComponents)
            {
                const int32 SlotCount = MeshComponent->GetNumMaterials();
                for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
                {
                    MeshComponent->SetMaterial(SlotIndex, Material);
                }
            }
            return FUnrealMCPCommonUtils::CreateSuccessResponse();
        }
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}
