#include "Commands/UnrealMCPCommonUtils.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Selection.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabase.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// JSON Utilities
TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateErrorResponse(const FString& Message)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), false);
    ResponseObject->SetStringField(TEXT("error"), Message);
    return ResponseObject;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), true);
    
    if (Data.IsValid())
    {
        ResponseObject->SetObjectField(TEXT("data"), Data);
    }
    
    return ResponseObject;
}

void FUnrealMCPCommonUtils::GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((int32)Value->AsNumber());
        }
    }
}

void FUnrealMCPCommonUtils::GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((float)Value->AsNumber());
        }
    }
}

FVector2D FUnrealMCPCommonUtils::GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector2D Result(0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 2)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
    }
    
    return Result;
}

FVector FUnrealMCPCommonUtils::GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
        Result.Z = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

FRotator FUnrealMCPCommonUtils::GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FRotator Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.Pitch = (float)(*JsonArray)[0]->AsNumber();
        Result.Yaw = (float)(*JsonArray)[1]->AsNumber();
        Result.Roll = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

// Blueprint Utilities
UBlueprint* FUnrealMCPCommonUtils::FindBlueprint(const FString& BlueprintName)
{
    return FindBlueprintByName(BlueprintName);
}

UBlueprint* FUnrealMCPCommonUtils::FindBlueprintByName(const FString& BlueprintName)
{
    FString AssetPath = TEXT("/Game/Blueprints/") + BlueprintName;
    return LoadObject<UBlueprint>(nullptr, *AssetPath);
}

UEdGraph* FUnrealMCPCommonUtils::FindOrCreateEventGraph(UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Try to find the event graph
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph->GetName().Contains(TEXT("EventGraph")))
        {
            return Graph;
        }
    }
    
    // Create a new event graph if none exists
    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(TEXT("EventGraph")), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewGraph);
    return NewGraph;
}

// Blueprint node utilities - FIXED class hierarchy search (Proposal #4)
UK2Node_Event* FUnrealMCPCommonUtils::CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Check for existing event node with this exact name
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Using existing event node with name %s (ID: %s)"), 
                *EventName, *EventNode->NodeGuid.ToString());
            return EventNode;
        }
    }

    // No existing node found, create a new one
    UK2Node_Event* EventNode = nullptr;
    
    // FIXED: Search the FULL class hierarchy for the event function (Proposal #4)
    // This handles inherited events like ReceiveBeginPlay, ReceiveTick, etc.
    UClass* SearchClass = Blueprint->GeneratedClass;
    UFunction* EventFunction = nullptr;
    
    while (SearchClass && !EventFunction)
    {
        EventFunction = SearchClass->FindFunctionByName(FName(*EventName));
        if (!EventFunction)
        {
            SearchClass = SearchClass->GetSuperClass();
        }
    }
    
    // If still not found, try the parent class of the blueprint
    if (!EventFunction && Blueprint->ParentClass)
    {
        SearchClass = Blueprint->ParentClass;
        while (SearchClass && !EventFunction)
        {
            EventFunction = SearchClass->FindFunctionByName(FName(*EventName));
            if (!EventFunction)
            {
                SearchClass = SearchClass->GetSuperClass();
            }
        }
    }
    
    if (EventFunction)
    {
        EventNode = NewObject<UK2Node_Event>(Graph);
        EventNode->EventReference.SetExternalMember(FName(*EventName), EventFunction->GetOwnerClass());
        EventNode->NodePosX = Position.X;
        EventNode->NodePosY = Position.Y;
        Graph->AddNode(EventNode, true);
        EventNode->PostPlacedNewNode();
        EventNode->AllocateDefaultPins();
        UE_LOG(LogTemp, Display, TEXT("Created new event node with name %s (ID: %s) on class %s"), 
            *EventName, *EventNode->NodeGuid.ToString(), *EventFunction->GetOwnerClass()->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find function for event name: %s in blueprint %s or its parent classes"), 
            *EventName, *Blueprint->GetName());
    }
    
    return EventNode;
}

UK2Node_CallFunction* FUnrealMCPCommonUtils::CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& Position)
{
    if (!Graph || !Function)
    {
        return nullptr;
    }
    
    UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
    FunctionNode->SetFromFunction(Function);
    FunctionNode->NodePosX = Position.X;
    FunctionNode->NodePosY = Position.Y;
    Graph->AddNode(FunctionNode, true);
    FunctionNode->CreateNewGuid();
    FunctionNode->PostPlacedNewNode();
    FunctionNode->AllocateDefaultPins();
    
    return FunctionNode;
}

UK2Node_VariableGet* FUnrealMCPCommonUtils::CreateVariableGetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableGet* VariableGetNode = NewObject<UK2Node_VariableGet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableGetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableGetNode->NodePosX = Position.X;
        VariableGetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableGetNode, true);
        VariableGetNode->PostPlacedNewNode();
        VariableGetNode->AllocateDefaultPins();
        
        return VariableGetNode;
    }
    
    return nullptr;
}

UK2Node_VariableSet* FUnrealMCPCommonUtils::CreateVariableSetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    if (!Graph || !Blueprint)
    {
        return nullptr;
    }
    
    UK2Node_VariableSet* VariableSetNode = NewObject<UK2Node_VariableSet>(Graph);
    
    FName VarName(*VariableName);
    FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
    
    if (Property)
    {
        VariableSetNode->VariableReference.SetFromField<FProperty>(Property, false);
        VariableSetNode->NodePosX = Position.X;
        VariableSetNode->NodePosY = Position.Y;
        Graph->AddNode(VariableSetNode, true);
        VariableSetNode->PostPlacedNewNode();
        VariableSetNode->AllocateDefaultPins();
        
        return VariableSetNode;
    }
    
    return nullptr;
}

UK2Node_InputAction* FUnrealMCPCommonUtils::CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_InputAction* InputActionNode = NewObject<UK2Node_InputAction>(Graph);
    InputActionNode->InputActionName = FName(*ActionName);
    InputActionNode->NodePosX = Position.X;
    InputActionNode->NodePosY = Position.Y;
    Graph->AddNode(InputActionNode, true);
    InputActionNode->CreateNewGuid();
    InputActionNode->PostPlacedNewNode();
    InputActionNode->AllocateDefaultPins();
    
    return InputActionNode;
}

UK2Node_Self* FUnrealMCPCommonUtils::CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
    SelfNode->NodePosX = Position.X;
    SelfNode->NodePosY = Position.Y;
    Graph->AddNode(SelfNode, true);
    SelfNode->CreateNewGuid();
    SelfNode->PostPlacedNewNode();
    SelfNode->AllocateDefaultPins();
    
    return SelfNode;
}

bool FUnrealMCPCommonUtils::ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, 
                                           UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    if (!Graph || !SourceNode || !TargetNode)
    {
        return false;
    }
    
    UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);
    
    if (SourcePin && TargetPin)
    {
        SourcePin->MakeLinkTo(TargetPin);
        return true;
    }
    
    return false;
}

UEdGraphPin* FUnrealMCPCommonUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    if (!Node)
    {
        return nullptr;
    }
    
    // Log all pins for debugging
    UE_LOG(LogTemp, Display, TEXT("FindPin: Looking for pin '%s' (Direction: %d) in node '%s'"), 
           *PinName, (int32)Direction, *Node->GetName());
    
    for (UEdGraphPin* Pin : Node->Pins)
    {
        UE_LOG(LogTemp, Display, TEXT("  - Available pin: '%s