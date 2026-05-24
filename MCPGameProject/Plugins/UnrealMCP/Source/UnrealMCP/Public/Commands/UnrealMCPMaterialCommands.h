#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for material MCP commands:
 *   - create_material           : create a new UMaterial asset
 *   - create_material_instance  : create a UMaterialInstanceConstant from a parent material
 *   - set_material_parameter    : set a scalar/vector parameter on a material instance
 *   - assign_material           : assign a material to a level actor's mesh component slot
 */
class UNREALMCP_API FUnrealMCPMaterialCommands
{
public:
    FUnrealMCPMaterialCommands();

    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    TSharedPtr<FJsonObject> HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetMaterialParameter(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAssignMaterial(const TSharedPtr<FJsonObject>& Params);
};
