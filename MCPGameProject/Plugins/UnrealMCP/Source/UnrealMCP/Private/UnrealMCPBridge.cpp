#include "UnrealMCPBridge.h"
#include "MCPServerRunnable.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "HAL/RunnableThread.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "JsonObjectConverter.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
// Add Blueprint related includes
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
// UE5.5 correct includes
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/Field.h"
#include "UObject/FieldPath.h"
// Blueprint Graph specific includes
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "GameFramework/InputSettings.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
// Include our new command handler classes
#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPBlueprintCommands.h"
#include "Commands/UnrealMCPBlueprintNodeCommands.h"
#include "Commands/UnrealMCPProjectCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Commands/UnrealMCPUMGCommands.h"
#include "Commands/UnrealMCPSystemCommands.h"
#include "Commands/UnrealMCPLevelCommands.h"
#include "Commands/UnrealMCPAssetCommands.h"
#include "Commands/UnrealMCPMaterialCommands.h"

// Default settings
#define MCP_SERVER_HOST "127.0.0.1"
#define MCP_SERVER_PORT 55557

UUnrealMCPBridge::UUnrealMCPBridge()
{
    EditorCommands = MakeShared<FUnrealMCPEditorCommands>();
    BlueprintCommands = MakeShared<FUnrealMCPBlueprintCommands>();
    BlueprintNodeCommands = MakeShared<FUnrealMCPBlueprintNodeCommands>();
    ProjectCommands = MakeShared<FUnrealMCPProjectCommands>();
    UMGCommands = MakeShared<FUnrealMCPUMGCommands>();
    SystemCommands = MakeShared<FUnrealMCPSystemCommands>();
    LevelCommands = MakeShared<FUnrealMCPLevelCommands>();
    AssetCommands = MakeShared<FUnrealMCPAssetCommands>();
    MaterialCommands = MakeShared<FUnrealMCPMaterialCommands>();

    // Build the self-registering command registry (Proposal #1)
    BuildCommandRegistry();
}

UUnrealMCPBridge::~UUnrealMCPBridge()
{
    EditorCommands.Reset();
    BlueprintCommands.Reset();
    BlueprintNodeCommands.Reset();
    ProjectCommands.Reset();
    UMGCommands.Reset();
    SystemCommands.Reset();
    LevelCommands.Reset();
    AssetCommands.Reset();
    MaterialCommands.Reset();
}

// Initialize subsystem
void UUnrealMCPBridge::Initialize(FSubsystemCollectionBase& Collection)
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Initializing"));
    
    bIsRunning = false;
    ListenerSocket = nullptr;
    ConnectionSocket = nullptr;
    ServerThread = nullptr;
    Port = MCP_SERVER_PORT;
    FIPv4Address::Parse(MCP_SERVER_HOST, ServerAddress);

    // Start the server automatically
    StartServer();
}

// Clean up resources when subsystem is destroyed
void UUnrealMCPBridge::Deinitialize()
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Shutting down"));
    StopServer();
}

// Start the MCP server
void UUnrealMCPBridge::StartServer()
{
    if (bIsRunning)
    {
        UE_LOG(LogTemp, Warning, TEXT("UnrealMCPBridge: Server is already running"));
        return;
    }

    // Create socket subsystem
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to get socket subsystem"));
        return;
    }

    // Create listener socket
    TSharedPtr<FSocket> NewListenerSocket = MakeShareable(SocketSubsystem->CreateSocket(NAME_Stream, TEXT("UnrealMCPListener"), false));
    if (!NewListenerSocket.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to create listener socket"));
        return;
    }

    // Allow address reuse for quick restarts
    NewListenerSocket->SetReuseAddr(true);
    NewListenerSocket->SetNonBlocking(true);

    // Bind to address
    FIPv4Endpoint Endpoint(ServerAddress, Port);
    if (!NewListenerSocket->Bind(*Endpoint.ToInternetAddr()))
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to bind listener socket to %s:%d"), *ServerAddress.ToString(), Port);
        return;
    }

    // Start listening
    if (!NewListenerSocket->Listen(5))
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to start listening"));
        return;
    }

    ListenerSocket = NewListenerSocket;
    bIsRunning = true;
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Server started on %s:%d"), *ServerAddress.ToString(), Port);

    // Start server thread
    ServerThread = FRunnableThread::Create(
        new FMCPServerRunnable(this, ListenerSocket),
        TEXT("UnrealMCPServerThread"),
        0, TPri_Normal
    );

    if (!ServerThread)
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to create server thread"));
        StopServer();
        return;
    }
}

// Stop the MCP server
void UUnrealMCPBridge::StopServer()
{
    if (!bIsRunning)
    {
        return;
    }

    bIsRunning = false;

    // Clean up thread
    if (ServerThread)
    {
        ServerThread->Kill(true);
        delete ServerThread;
        ServerThread = nullptr;
    }

    // Close sockets
    if (ConnectionSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket.Get());
        ConnectionSocket.Reset();
    }

    if (ListenerSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket.Get());
        ListenerSocket.Reset();
    }

    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Server stopped"));
}

// Build the self-registering command registry (Proposal #1)
void UUnrealMCPBridge::BuildCommandRegistry()
{
    // Editor Commands
    const TArray<FString> EditorCmds = {
        TEXT("ping"),
        TEXT("get_actors_in_level"), TEXT("find_actors_by_name"),
        TEXT("spawn_actor"), TEXT("create_actor"), TEXT("delete_actor"),
        TEXT("set_actor_transform"), TEXT("get_actor_properties"), TEXT("set_actor_property"),
        TEXT("spawn_blueprint_actor"), TEXT("focus_viewport"), TEXT("take_screenshot")
    };
    for (const FString& Cmd : EditorCmds)
    {
        SupportedCommands.Add(Cmd);
        CommandToCategoryMap.Add(Cmd, TEXT("editor"));
    }

    // Blueprint Commands
    const TArray<FString> BlueprintCmds = {
        TEXT("create_blueprint"), TEXT("add_component_to_blueprint"),
        TEXT("set_component_property"), TEXT("set_physics_properties"),
        TEXT("compile_blueprint"), TEXT("set_blueprint_property"),
        TEXT("set_static_mesh_properties"), TEXT("set_pawn_properties"),
        TEXT("save_blueprint"), TEXT("is_blueprint_dirty")
    };
    for (const FString& Cmd : BlueprintCmds)
    {
        SupportedCommands.Add(Cmd);
        CommandToCategoryMap.Add(Cmd, TEXT("blueprint"));
    }

    // Blueprint Node Commands
    const TArray<FString> BlueprintNodeCmds = {
        TEXT("connect_blueprint_nodes"), TEXT("add_blueprint_get_self_component_reference"),
        TEXT("add_blueprint_self_reference"), TEXT("find_blueprint_nodes"),
        TEXT("add_blueprint_event_node"), TEXT("add_blueprint_input_action_node"),
        TEXT("add_blueprint_function_node"), TEXT("add_blueprint_variable")
    };
    for (const FString& Cmd : BlueprintNodeCmds)
    {
        SupportedCommands.Add(Cmd);
        CommandToCategoryMap.Add(Cmd, TEXT("blueprint_node"));
    }

    // Project Commands
    const TArray<FString> ProjectCmds = {
        TEXT("create_input_mapping")
    };
    for (const FString& Cmd : ProjectCmds)
    {
        SupportedCommands.Add(Cmd);
        CommandToCategoryMap.Add(Cmd, TEXT("project"));
    }

    // UMG Commands
    const TArray<FString> UMGCmds = {
        TEXT("create_umg_widget_blueprint"), TEXT("add_text_block_to_widget"),
        TEXT("add_button_to_widget"), TEXT("bind_widget_event"),
        TEXT("set_text_block_binding"), TEXT("add_widget_to_viewport")
    };
    for (const FString& Cmd : UMGCmds)
    {
        SupportedCommands.Add(Cmd);
        CommandToCategoryMap.Add(Cmd, TEXT("umg"));
    }

    // System Commands
    const TArray<FString> SystemCmds = {
        TEXT("execute_python"), TEXT("execute_python_file"),
        TEXT("execute_console_command"),
        TEXT("get_class_info"), TEXT("list_assets"),
        TEXT("get_supported_commands")
    };
    for (const FString& Cmd : SystemCmds)
    {
        SupportedCommands.Add(Cmd);
        CommandToCategoryMap.Add(Cmd, TEXT("system"));
    }

    // Level Commands
    const TArray<FString> LevelCmds = {
        TEXT("open_level"), TEXT("save_current_level"),
        TEXT("save_all"), TEXT("new_level"), TEXT("get_current_level")
    };
    for (const FString& Cmd : LevelCmds)
    {
        SupportedCommands.Add(Cmd);
        CommandToCategoryMap.Add(Cmd, TEXT("level"));
    }

    // Asset Commands
    const TArray<FString> AssetCmds = {
        TEXT("import_asset"), TEXT("duplicate_asset"),
        TEXT("rename_asset"), TEXT("delete_asset"),
        TEXT("save_asset"), TEXT("create_folder")
    };
    for (const FString& Cmd : AssetCmds)
    {
        SupportedCommands.Add(Cmd);
        CommandToCategoryMap.Add(Cmd, TEXT("asset"));
    }

    // Material Commands
    const TArray<FString> MaterialCmds = {
        TEXT("create_material"), TEXT("create_material_instance"),
        TEXT("set_material_parameter"), TEXT("assign_material"),
        TEXT("set_material_color"), TEXT("get_material_info"),
        TEXT("assign_material_to_all_slots")
    };
    for (const FString& Cmd : MaterialCmds)
    {
        SupportedCommands.Add(Cmd);
        CommandToCategoryMap.Add(Cmd, TEXT("material"));
    }
}

// Runtime command discovery (Proposal #2)
TArray<FString> UUnrealMCPBridge::GetSupportedCommands() const
{
    TArray<FString> Commands;
    Commands.Reserve(SupportedCommands.Num());
    for (const FString& Command : SupportedCommands)
    {
        Commands.Add(Command);
    }
    Commands.Sort();
    return Commands;
}

FString UUnrealMCPBridge::GetCommandCategory(const FString& CommandType) const
{
    const FString* Category = CommandToCategoryMap.Find(CommandType);
    return Category ? *Category : TEXT("unknown");
}

// Command suggestion engine (Proposal #7)
FString UUnrealMCPBridge::SuggestCommand(const FString& UnknownCommand) const
{
    FString BestMatch;
    int32 BestDistance = INT32_MAX;

    for (const FString& Cmd : SupportedCommands)
    {
        int32 Dist = LevenshteinDistance(UnknownCommand, Cmd);
        if (Dist < BestDistance)
        {
            BestDistance = Dist;
            BestMatch = Cmd;
        }
    }

    // Only suggest if reasonably close (threshold = 5)
    if (BestDistance <= 5 && !BestMatch.IsEmpty())
    {
        return FString::Printf(TEXT("Did you mean '%s'?"), *BestMatch);
    }
    return TEXT("");
}

int32 UUnrealMCPBridge::LevenshteinDistance(const FString& A, const FString& B) const
{
    const int32 LenA = A.Len();
    const int32 LenB = B.Len();

    if (LenA == 0) return LenB;
    if (LenB == 0) return LenA;

    TArray<int32> PrevRow, CurrRow;
    PrevRow.SetNum(LenB + 1);
    CurrRow.SetNum(LenB + 1);

    for (int32 j = 0; j <= LenB; ++j)
    {
        PrevRow[j] = j;
    }

    for (int32 i = 1; i <= LenA; ++i)
    {
        CurrRow[0] = i;
        for (int32 j = 1; j <= LenB; ++j)
        {
            int32 Cost = (A[i - 1] == B[j - 1]) ? 0 : 1;
            CurrRow[j] = FMath::Min3(
                PrevRow[j] + 1,      // deletion
                CurrRow[j - 1] + 1,  // insertion
                PrevRow[j - 1] + Cost // substitution
            );
        }
        Swap(PrevRow, CurrRow);
    }

    return PrevRow[LenB];
}

// Execute a command received from a client
FString UUnrealMCPBridge::ExecuteCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Executing command: %s"), *CommandType);
    
    // Create a promise to wait for the result
    TPromise<FString> Promise;
    TFuture<FString> Future = Promise.GetFuture();
    
    // Queue execution on Game Thread
    AsyncTask(ENamedThreads::GameThread, [this, CommandType, Params, Promise = MoveTemp(Promise)]() mutable
    {
        TSharedPtr<FJsonObject> ResponseJson = MakeShareable(new FJsonObject);
        
        try
        {
            TSharedPtr<FJsonObject> ResultJson;
            
            // Use the self-registering command map for dispatch (Proposal #1)
            const FString Category = GetCommandCategory(CommandType);
            
            if (CommandType == TEXT("ping"))
            {
                ResultJson = MakeShareable(new FJsonObject);
                ResultJson->SetStringField(TEXT("message"), TEXT("pong"));
            }
            else if (CommandType == TEXT("get_supported_commands"))
            {
                // Runtime command discovery (Proposal #2) — answered by the bridge
                // itself so the list always matches the dispatch registry.
                ResultJson = MakeShareable(new FJsonObject);
                TArray<TSharedPtr<FJsonValue>> CommandArray;
                for (const FString& Cmd : GetSupportedCommands())
                {
                    TSharedPtr<FJsonObject> CmdObj = MakeShared<FJsonObject>();
                    CmdObj->SetStringField(TEXT("name"), Cmd);
                    CmdObj->SetStringField(TEXT("category"), GetCommandCategory(Cmd));
                    CommandArray.Add(MakeShared<FJsonValueObject>(CmdObj));
                }
                ResultJson->SetArrayField(TEXT("commands"), CommandArray);
                ResultJson->SetNumberField(TEXT("count"), CommandArray.Num());
            }
            else if (Category == TEXT("editor"))
            {
                ResultJson = EditorCommands->HandleCommand(CommandType, Params);
            }
            else if (Category == TEXT("blueprint"))
            {
                ResultJson = BlueprintCommands->HandleCommand(CommandType, Params);
            }
            else if (Category == TEXT("blueprint_node"))
            {
                ResultJson = BlueprintNodeCommands->HandleCommand(CommandType, Params);
            }
            else if (Category == TEXT("project"))
            {
                ResultJson = ProjectCommands->HandleCommand(CommandType, Params);
            }
            else if (Category == TEXT("umg"))
            {
                ResultJson = UMGCommands->HandleCommand(CommandType, Params);
            }
            else if (Category == TEXT("system"))
            {
                ResultJson = SystemCommands->HandleCommand(CommandType, Params);
            }
            else if (Category == TEXT("level"))
            {
                ResultJson = LevelCommands->HandleCommand(CommandType, Params);
            }
            else if (Category == TEXT("asset"))
            {
                ResultJson = AssetCommands->HandleCommand(CommandType, Params);
            }
            else if (Category == TEXT("material"))
            {
                ResultJson = MaterialCommands->HandleCommand(CommandType, Params);
            }
            else
            {
                // Unknown command - provide suggestions (Proposal #7)
                FString Suggestion = SuggestCommand(CommandType);
                FString ErrorMsg = FString::Printf(TEXT("Unknown command: %s"), *CommandType);
                if (!Suggestion.IsEmpty())
                {
                    ErrorMsg += TEXT(" ") + Suggestion;
                }
                
                ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                ResponseJson->SetStringField(TEXT("error"), ErrorMsg);
                
                FString ResultString;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
                FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
                Promise.SetValue(ResultString);
                return;
            }
            
            // Check if the result contains an error
            bool bSuccess = true;
            FString ErrorMessage;
            
            if (ResultJson->HasField(TEXT("success")))
            {
                bSuccess = ResultJson->GetBoolField(TEXT("success"));
                if (!bSuccess && ResultJson->HasField(TEXT("error")))
                {
                    ErrorMessage = ResultJson->GetStringField(TEXT("error"));
                }
            }
            
            if (bSuccess)
            {
                // Set success status and include the result
                ResponseJson->SetStringField(TEXT("status"), TEXT("success"));
                ResponseJson->SetObjectField(TEXT("result"), ResultJson);
            }
            else
            {
                // Set error status and include the error message
                ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                ResponseJson->SetStringField(TEXT("error"), ErrorMessage);
            }
        }
        catch (const std::exception& e)
        {
            ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
            ResponseJson->SetStringField(TEXT("error"), UTF8_TO_TCHAR(e.what()));
        }
        
        FString ResultString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
        FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
        Promise.SetValue(ResultString);
    });
    
    return Future.Get();
}