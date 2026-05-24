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
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name', 'path' or 'parent_material' parameter"));
    }

    UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, *ParentPath);
    if (!Parent)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Parent material not found: %s"), *ParentPath));
    }

    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
    Factory->InitialParent = Parent;

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
    if (!Params->TryGetStringField(TEXT("material"), MaterialPath) ||
        !Params->TryGetStringField(TEXT("parameter"), ParameterName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material' or 'parameter' parameter"));
    }

    UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *MaterialPath);
    if (!Instance)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material instance not found (parameters can only be set on instances): %s"), *MaterialPath));
    }

    const FName ParamName(*ParameterName);

    // A "value" array of length >= 3 is treated as a vector/color; a scalar otherwise.
    const TArray<TSharedPtr<FJsonValue>>* VectorValue = nullptr;
    if (Params->TryGetArrayField(TEXT("value"), VectorValue) && VectorValue && VectorValue->Num() >= 3)
    {
        const float R = (*VectorValue)[0]->AsNumber();
        const float G = (*VectorValue)[1]->AsNumber();
        const float B = (*VectorValue)[2]->AsNumber();
        const float A = VectorValue->Num() >= 4 ? (*VectorValue)[3]->AsNumber() : 1.0f;
        UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(Instance, ParamName, FLinearColor(R, G, B, A));
    }
    else if (Params->HasField(TEXT("value")))
    {
        const float ScalarValue = Params->GetNumberField(TEXT("value"));
        UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(Instance, ParamName, ScalarValue);
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter (number for scalar, [r,g,b,(a)] for vector)"));
    }

    UMaterialEditingLibrary::UpdateMaterialInstance(Instance);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("material"), MaterialPath);
    ResultObj->SetStringField(TEXT("parameter"), ParameterName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleAssignMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName, MaterialPath;
    if (!Params->TryGetStringField(TEXT("actor"), ActorName) ||
        !Params->TryGetStringField(TEXT("material"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor' or 'material' parameter"));
    }

    int32 SlotIndex = 0;
    if (Params->HasField(TEXT("slot")))
    {
        SlotIndex = (int32)Params->GetNumberField(TEXT("slot"));
    }

    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    if (!Material)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active editor world"));
    }

    // Find the level actor by name.
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }
    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    UMeshComponent* MeshComponent = TargetActor->FindComponentByClass<UMeshComponent>();
    if (!MeshComponent)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor '%s' has no mesh component"), *ActorName));
    }

    MeshComponent->SetMaterial(SlotIndex, Material);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("actor"), ActorName);
    ResultObj->SetStringField(TEXT("material"), MaterialPath);
    ResultObj->SetNumberField(TEXT("slot"), SlotIndex);
    return ResultObj;
}
