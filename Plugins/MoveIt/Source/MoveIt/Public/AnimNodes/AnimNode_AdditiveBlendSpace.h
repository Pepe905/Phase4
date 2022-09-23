// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "UnrealBase/AnimNode_MIBlendSpacePlayer.h"
#include "AnimNodes/AnimNode_ModifyCurve.h"
#include "Animation/InputScaleBias.h"
#include "AnimNode_AdditiveBlendSpace.generated.h"

UENUM(BlueprintType)
enum class EMIAdditiveType : uint8
{
	MIAS_Additive					UMETA(DisplayName = "Additive"),
	MIAS_MeshSpaceAdditive			UMETA(DisplayName = "Mesh Space Additive"),
};

/**
 *	Equivalent of "Apply Additive" with a blendspace and has additional blending options
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_AdditiveBlendSpace : public FAnimNode_MIBlendSpacePlayer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink SourcePose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	EMIAdditiveType AdditiveType;

	FName GetAdditiveName() const { return (AdditiveType == EMIAdditiveType::MIAS_Additive) ? TEXT("Additive") : TEXT("Mesh Space"); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Alpha, meta=(PinShownByDefault))
	float Alpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Alpha)
	FInputScaleBias AlphaScaleBias;

	/* 
	 * Max LOD that this node is allowed to run
	 * For example if you have LODThreadhold to be 2, it will run until LOD 2 (based on 0 index)
	 * when the component LOD becomes 3, it will stop update/evaluate
	 * currently transition would be issue and that has to be re-visited
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta=(DisplayName="LOD Threshold"))
	int32 LODThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha, meta = (DisplayName = "Blend Settings", DisplayAfter = "bAlphaBoolEnabled"))
	FInputAlphaBoolBlend AlphaBoolBlend;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha, meta = (PinShownByDefault))
	FName AlphaCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha)
	FInputScaleBiasClamp AlphaScaleBiasClamp;

	float ActualAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha, meta = (DisplayAfter = "AlphaScaleBias"))
	EAnimAlphaInputType AlphaInputType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha, meta = (PinShownByDefault, DisplayName = "bEnabled", DisplayAfter = "AlphaInputType"))
	bool bAlphaBoolEnabled;

public:
	FAnimNode_AdditiveBlendSpace()
		: AdditiveType(EMIAdditiveType::MIAS_Additive)
		, Alpha(1.0f)
		, LODThreshold(INDEX_NONE)
		, AlphaCurveName(NAME_None)
		, ActualAlpha(0.f)
		, AlphaInputType(EAnimAlphaInputType::Float)
		, bAlphaBoolEnabled(true)
	{}

public:
	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual int32 GetLODThreshold() const override { return LODThreshold; }
	// End of FAnimNode_Base interface

protected:
	virtual void UpdateBlendSpace(const FAnimationUpdateContext& Context) {}

private:
	void UpdateInternal(const FAnimationUpdateContext& Context);
};