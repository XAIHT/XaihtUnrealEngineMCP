#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Http.h"
#include "Json.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPBlueprintCommands.h"
#include "Commands/UnrealMCPBlueprintNodeCommands.h"
#include "Commands/UnrealMCPProjectCommands.h"
#include "Commands/UnrealMCPUMGCommands.h"
#include "Commands/UnrealMCPSystemCommands.h"
#include "Commands/UnrealMCPLevelCommands.h"
#include "Commands/UnrealMCPAssetCommands.h"
#include "Commands/UnrealMCPMaterialCommands.h"
#include "UnrealMCPBridge.generated.h"

class FMCPServerRunnable;

/**
 * Editor subsystem for MCP Bridge
 * Handles communication between external tools and the Unreal Editor
 * through a TCP socket connection. Commands are received as JSON and
 * routed to appropriate command handlers.
 * 
 * v2.0 - Self-registering command map with runtime validation,
 *        command suggestions, and comprehensive error reporting.
 */
UCLASS()
class UNREALMCP_API UUnrealMCPBridge : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UUnrealMCPBridge();
	virtual ~UUnrealMCPBridge();

	// UEditorSubsystem implementation
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Server functions
	void StartServer();
	void StopServer();
	bool IsRunning() const { return bIsRunning; }

	// Command execution
	FString ExecuteCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	// Runtime command discovery (Proposal #2)
	TArray<FString> GetSupportedCommands() const;
	FString GetCommandCategory(const FString& CommandType) const;

private:
	// Server state
	bool bIsRunning;
	TSharedPtr<FSocket> ListenerSocket;
	TSharedPtr<FSocket> ConnectionSocket;
	FRunnableThread* ServerThread;

	// Server configuration
	FIPv4Address ServerAddress;
	uint16 Port;

	// Command handler instances
	TSharedPtr<FUnrealMCPEditorCommands> EditorCommands;
	TSharedPtr<FUnrealMCPBlueprintCommands> BlueprintCommands;
	TSharedPtr<FUnrealMCPBlueprintNodeCommands> BlueprintNodeCommands;
	TSharedPtr<FUnrealMCPProjectCommands> ProjectCommands;
	TSharedPtr<FUnrealMCPUMGCommands> UMGCommands;
	TSharedPtr<FUnrealMCPSystemCommands> SystemCommands;
	TSharedPtr<FUnrealMCPLevelCommands> LevelCommands;
	TSharedPtr<FUnrealMCPAssetCommands> AssetCommands;
	TSharedPtr<FUnrealMCPMaterialCommands> MaterialCommands;

	// Self-registering command map (Proposal #1)
	TMap<FString, FString> CommandToCategoryMap;
	TSet<FString> SupportedCommands;
	void BuildCommandRegistry();

	// Command suggestion engine (Proposal #7)
	FString SuggestCommand(const FString& UnknownCommand) const;
	int32 LevenshteinDistance(const FString& A, const FString& B) const;
};