// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "AnimNodes/AnimNode_AdditiveBlendSpace.h"
#include "Animation/BlendSpace.h"
#include "AnimGraphNode_AdditiveBlendSpace.generated.h"

UCLASS(Abstract)
class MOVEITEDITOR_API UAnimGraphNode_AdditiveBlendSpace : public UAnimGraphNode_Base
{
	GENERATED_BODY()

public:
	// Sync group settings for this player.  Sync groups keep related animations with different lengths synchronized.
	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimationGroupReference SyncGroup;

public:
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FString GetNodeCategory() const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UEdGraphNode interface

	virtual FText GetControllerDescription() const;

	// UAnimGraphNode_Base interface
	virtual void ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog) override;
	virtual void BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog) override;
	virtual bool DoesSupportTimeForTransitionGetter() const override;
	virtual UAnimationAsset* GetAnimationAsset() const override;
	virtual const TCHAR* GetTimePropertyName() const override;
	virtual UScriptStruct* GetTimePropertyStruct() const override;
	virtual void GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets) const override;
	virtual void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& AnimAssetReplacementMap) override;
	virtual EAnimAssetHandlerType SupportsAssetClass(const UClass* AssetClass) const override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const override;
	virtual void PreloadRequiredAssets() override;
	virtual void PostProcessPinName(const UEdGraphPin* Pin, FString& DisplayName) const override;
	// End of UAnimGraphNode_Base interface

	// UAnimGraphNode_AssetPlayerBase interface
	virtual void SetAnimationAsset(UAnimationAsset* Asset);
	// End of UAnimGraphNode_AssetPlayerBase interface

protected:
	virtual FAnimNode_AdditiveBlendSpace* GetNode() PURE_VIRTUAL(UAnimGraphNode_AdditiveBlendSpace::GetNode, return nullptr;);
	virtual const FAnimNode_AdditiveBlendSpace* GetNode() const PURE_VIRTUAL(UAnimGraphNode_AdditiveBlendSpace::GetNode, return nullptr;);

	UBlendSpace* GetBlendSpace() const { return Cast<UBlendSpace>(GetAnimationAsset()); }

	/** Util to determine is an asset class is an aim offset */
	static bool IsAimOffsetBlendSpace(const UClass* BlendSpaceClass);

private:
	/** Constructing FText strings can be costly, so we cache the node's title */
	FNodeTitleTextTable CachedNodeTitles;
};