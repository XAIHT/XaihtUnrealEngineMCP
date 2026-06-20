#include "Commands/UnrealMCPSystemCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "IPythonScriptPlugin.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FUnrealMCPSystemCommands::FUnrealMCPSystemCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPSystemCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("execute_python"))
    {
        return HandleExecutePython(Params);
    }
    else if (CommandType == TEXT("execute_python_file"))
    {
        return HandleExecutePythonFile(Params);
    }
    else if (CommandType == TEXT("execute_console_command"))
    {
        return HandleExecuteConsoleCommand(Params);
    }
    else if (CommandType == TEXT("get_class_info"))
    {
        return HandleGetClassInfo(Params);
    }
    else if (CommandType == TEXT("list_assets"))
    {
        return HandleListAssets(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown system command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPSystemCommands::HandleExecutePython(const TSharedPtr<FJsonObject>& Params)
{
    // Accept either "code" or "command" as the script source.
    FString Code;
    if (!Params->TryGetStringField(TEXT("code"), Code) && !Params->TryGetStringField(TEXT("command"), Code))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'code' parameter"));
    }

    IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
    if (!PythonPlugin || !PythonPlugin->IsPythonAvailable())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("Python scripting is not available. Enable the 'Python Editor Script Plugin' for this project."));
    }

    FPythonCommandEx PythonCommand;
    PythonCommand.Command = Code;
    // ExecuteFile mode runs the whole string as a script (supports multi-line code).
    PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
    PythonCommand.Flags = EPythonCommandFlags::Unattended;

    const bool bExecOk = PythonPlugin->ExecPythonCommandEx(PythonCommand);

    // Capture any log/stdout/stderr the script produced.
    TArray<TSharedPtr<FJsonValue>> LogEntries;
    for (const FPythonLogOutputEntry& Entry : PythonCommand.LogOutput)
    {
        TSharedPtr<FJsonObject> EntryObj = MakeShared<FJsonObject>();
        EntryObj->SetStringField(TEXT("type"), Entry.Type == EPythonLogOutputType::Error ? TEXT("error") : TEXT("info"));
        EntryObj->SetStringField(TEXT("output"), Entry.Output);
        LogEntries.Add(MakeShared<FJsonValueObject>(EntryObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), bExecOk);
    ResultObj->SetStringField(TEXT("result"), PythonCommand.CommandResult);
    ResultObj->SetArrayField(TEXT("log"), LogEntries);
    if (!bExecOk)
    {
        ResultObj->SetStringField(TEXT("error"), TEXT("Python execution reported errors; see 'log' for details."));
    }
    return ResultObj;
}

// NEW: Execute Python from a file path (Proposal #6)
TSharedPtr<FJsonObject> FUnrealMCPSystemCommands::HandleExecutePythonFile(const TSharedPtr<FJsonObject>& Params)
{
    FString FilePath;
    if (!Params->TryGetStringField(TEXT("file_path"), FilePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'file_path' parameter"));
    }

    // Resolve relative paths against project directory
    if (FPaths::IsRelative(FilePath))
    {
        FilePath = FPaths::ProjectDir() / FilePath;
    }
    FPaths::NormalizeFilename(FilePath);

    if (!FPaths::FileExists(FilePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Python file not found: %s"), *FilePath));
    }

    FString Code;
    if (!FFileHelper::LoadFileToString(Code, *FilePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to read Python file: %s"), *FilePath));
    }

    IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
    if (!PythonPlugin || !PythonPlugin->IsPythonAvailable())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("Python scripting is not available. Enable the 'Python Editor Script Plugin' for this project."));
    }

    FPythonCommandEx PythonCommand;
    PythonCommand.Command = Code;
    PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
    PythonCommand.Flags = EPythonCommandFlags::Unattended;

    const bool bExecOk = PythonPlugin->ExecPythonCommandEx(PythonCommand);

    TArray<TSharedPtr<FJsonValue>> LogEntries;
    for (const FPythonLogOutputEntry& Entry : PythonCommand.LogOutput)
    {
        TSharedPtr<FJsonObject> EntryObj = MakeShared<FJsonObject>();
        EntryObj->SetStringField(TEXT("type"), Entry.Type == EPythonLogOutputType::Error ? TEXT("error") : TEXT("info"));
        EntryObj->SetStringField(TEXT("output"), Entry.Output);
        LogEntries.Add(MakeShared<FJsonValueObject>(EntryObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), bExecOk);
    ResultObj->SetStringField(TEXT("file"), FilePath);
    ResultObj->SetStringField(TEXT("result"), PythonCommand.CommandResult);
    ResultObj->SetArrayField(TEXT("log"), LogEntries);
    if (!bExecOk)
    {
        ResultObj->SetStringField(TEXT("error"), TEXT("Python execution reported errors; see 'log' for details."));
    }
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPSystemCommands::HandleExecuteConsoleCommand(const TSharedPtr<FJsonObject>& Params)
{
    FString Command;
    if (!Params->TryGetStringField(TEXT("command"), Command))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'command' parameter"));
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;

    bool bExecuted = false;
    if (GEngine)
    {
        bExecuted = GEngine->Exec(World, *Command);
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("executed"), bExecuted);
    ResultObj->SetStringField(TEXT("command"), Command);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPSystemCommands::HandleGetClassInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString ClassName;
    if (!Params->TryGetStringField(TEXT("class_name"), ClassName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'class_name' parameter"));
    }

    // UClass object names drop the engine prefix (e.g. "Actor" resolves to AActor).
    UClass* Class = UClass::TryFindTypeSlow<UClass>(ClassName);
    if (!Class)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Class not found: %s"), *ClassName));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), Class->GetName());
    ResultObj->SetStringField(TEXT("path"), Class->GetPathName());
    if (Class->GetSuperClass())
    {
        ResultObj->SetStringField(TEXT("parent"), Class->GetSuperClass()->GetName());
    }

    // Properties declared directly on this class (ExcludeSuper keeps the payload bounded).
    TArray<TSharedPtr<FJsonValue>> PropArray;
    for (TFieldIterator<FProperty> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        FProperty* Prop = *It;
        TSharedPtr<FJsonObject> PropObj = MakeShared<FJsonObject>();
        PropObj->SetStringField(TEXT("name"), Prop->GetName());
        PropObj->SetStringField(TEXT("type"), Prop->GetCPPType());
        PropArray.Add(MakeShared<FJsonValueObject>(PropObj));
    }
    ResultObj->SetArrayField(TEXT("properties"), PropArray);

    // Functions declared directly on this class.
    TArray<TSharedPtr<FJsonValue>> FuncArray;
    for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        FuncArray.Add(MakeShared<FJsonValueString>((*It)->GetName()));
    }
    ResultObj->SetArrayField(TEXT("functions"), FuncArray);

    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPSystemCommands::HandleListAssets(const TSharedPtr<FJsonObject>& Params)
{
    FString Path = TEXT("/Game");
    Params->TryGetStringField(TEXT("path"), Path);

    bool bRecursive = true;
    if (Params->HasField(TEXT("recursive")))
    {
        bRecursive = Params->GetBoolField(TEXT("recursive"));
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*Path));
    Filter.bRecursivePaths = bRecursive;

    TArray<FAssetData> Assets;
    AssetRegistry.GetAssets(Filter, Assets);

    TArray<TSharedPtr<FJsonValue>> AssetArray;
    for (const FAssetData& Asset : Assets)
    {
        TSharedPtr<FJsonObject> AssetObj = MakeShared<FJsonObject>();
        AssetObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
        AssetObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
        AssetObj->SetStringField(TEXT("class"), Asset.AssetClassPath.ToString());
        AssetArray.Add(MakeShared<FJsonValueObject>(AssetObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetNumberField(TEXT("count"), AssetArray.Num());
    ResultObj->SetArrayField(TEXT("assets"), AssetArray);
    return ResultObj;
}