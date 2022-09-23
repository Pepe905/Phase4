// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "AnimNodes/AnimNode_LocalSkeletalControlBase.h"
#include "AnimGraphNode_LocalSkeletalControlBase.generated.h"

struct FAnimNode_LocalSkeletalControlBase;

UCLASS(Abstract)
class MOVEITEDITOR_API UAnimGraphNode_LocalSkeletalControlBase : public UAnimGraphNode_Base
{
	GENERATED_BODY()

public:
	// UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const override;
	virtual void ValidateAnimNodePostCompile(FCompilerResultsLog& MessageLog, UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex) override;
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of UAnimGraphNode_Base interface

	/** Are we currently showing this pin */
	bool IsPinShown(const FName PinName) const;

public:
	// set literal value for FVector
	void SetDefaultValue(const FName InDefaultValueName, const FVector& InValue);
	// get literal value for vector
	void GetDefaultValue(const FName UpdateDefaultValueName, FVector& OutVec);

	void GetDefaultValue(const FName PropName, FRotator& OutValue)
	{
		FVector Value;
		GetDefaultValue(PropName, Value);
		OutValue.Pitch = Value.X;
		OutValue.Yaw = Value.Y;
		OutValue.Roll = Value.Z;
	}

	template<class ValueType>
	ValueType GetNodeValue(const FName PropName, const ValueType& CompileNodeValue)
	{
		if (IsPinShown(PropName))
		{
			ValueType Val;
			GetDefaultValue(PropName, Val);
			return Val;
		}
		return CompileNodeValue;
	}

	void SetDefaultValue(const FName PropName, const FRotator& InValue)
	{
		FVector VecValue(InValue.Pitch, InValue.Yaw, InValue.Roll);
		SetDefaultValue(PropName, VecValue);
	}

	template<class ValueType>
	void SetNodeValue(const FName PropName, ValueType& CompileNodeValue, const ValueType& InValue)
	{
		if (IsPinShown(PropName))
		{
			SetDefaultValue(PropName, InValue);
		}
		CompileNodeValue = InValue;
	}

protected:
	// Returns the short descriptive name of the controller
	virtual FText GetControllerDescription() const;

	virtual const FAnimNode_LocalSkeletalControlBase* GetNode() const PURE_VIRTUAL(UAnimGraphNode_LocalSkeletalControlBase::GetNode, return nullptr;);
};