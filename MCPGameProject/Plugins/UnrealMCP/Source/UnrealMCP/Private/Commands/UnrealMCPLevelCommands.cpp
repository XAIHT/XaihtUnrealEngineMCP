#include "Commands/UnrealMCPLevelCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/Level.h"
// UE 5.7: UEditorLoadingAndSavingUtils is declared in FileHelpers.h (the standalone
// EditorLoadingAndSavingUtils.h header no longer exists).
#include "FileHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"

FUnrealMCPLevelCommands::FUnrealMCPLevelCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("open_level"))
    {
        return HandleOpenLevel(Params);
    }
    else if (CommandType == TEXT("save_current_level"))
    {
        return HandleSaveCurrentLevel(Params);
    }
    else if (CommandType == TEXT("save_all"))
    {
        return HandleSaveAll(Params);
    }
    else if (CommandType == TEXT("new_level"))
    {
        return HandleNewLevel(Params);
    }
    else if (CommandType == TEXT("get_current_level"))
    {
        return HandleGetCurrentLevel(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown level command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleOpenLevel(const TSharedPtr<FJsonObject>& Params)
{
    FString Path;
    if (!Params->TryGetStringField(TEXT("path"), Path))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'path' parameter"));
    }

    // LoadMap prompts to save unsaved changes unless suppressed; the bridge runs on the
    // game thread so this is safe to call directly.
    UEditorLoadingAndSavingUtils::LoadMap(Path);

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("opened"), Path);
    if (World)
    {
        ResultObj->SetStringField(TEXT("current"), World->GetOutermost()->GetName());
    }
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleSaveCurrentLevel(const TSharedPtr<FJsonObject>& Params)
{
    const bool bSaved = UEditorLoadingAndSavingUtils::SaveCurrentLevel();

    if (!bSaved)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to save current level"));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("saved"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleSaveAll(const TSharedPtr<FJsonObject>& Params)
{
    // Save both map packages and content packages without an interactive prompt.
    const bool bSaved = UEditorLoadingAndSavingUtils::SaveDirtyPackages(/*bSaveMapPackages*/ true, /*bSaveContentPackages*/ true);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("saved"), bSaved);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleNewLevel(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* NewWorld = UEditorLoadingAndSavingUtils::NewBlankMap(/*bSaveExistingMap*/ false);
    if (!NewWorld)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create new level"));
    }

    // If a path was provided, save the new map there.
    FString Path;
    if (Params->TryGetStringField(TEXT("path"), Path) && !Path.IsEmpty())
    {
        const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(NewWorld, Path);
        if (!bSaved)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Created level but failed to save it to: %s"), *Path));
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), NewWorld->GetName());
    if (!Path.IsEmpty())
    {
        ResultObj->SetStringField(TEXT("path"), Path);
    }
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPLevelCommands::HandleGetCurrentLevel(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active editor world"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), World->GetName());
    ResultObj->SetStringField(TEXT("path"), World->GetOutermost()->GetName());
    ResultObj->SetNumberField(TEXT("actor_count"), AllActors.Num());
    return ResultObj;
}
