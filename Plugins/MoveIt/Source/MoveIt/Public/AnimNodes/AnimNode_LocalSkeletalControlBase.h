// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "Animation/AnimNodeBase.h"
#include "AnimNodes/AnimNodes_Shared.h"
#include "Animation/InputScaleBias.h"
#include "AnimNode_LocalSkeletalControlBase.generated.h"

struct MOVEIT_API FBoneLookupTable
{
	FBoneLookupTable()
		: BoneIndex(FCompactPoseBoneIndex(-1))
		, Cache(TArray<FCompactPoseBoneIndex>())
		, ParentComponentSpace(FTransform::Identity)
		, TargetLocalSpace(nullptr)
	{}

	~FBoneLookupTable(){}

	FCompactPoseBoneIndex BoneIndex;
	TArray<FCompactPoseBoneIndex> Cache;

	FTransform ParentComponentSpace;
	FTransform* TargetLocalSpace;

	FTransform ConvertLocalToComponent(FPoseContext& Output)
	{
		ParentComponentSpace = Output.Pose[Cache[0]].Inverse();

		for (int32 i = 1; i < Cache.Num(); i++)
		{
			FTransform NextParent = Output.Pose[Cache[i]].Inverse();
			FTransform::Multiply(&ParentComponentSpace, &ParentComponentSpace, &NextParent);
		}

		ParentComponentSpace = ParentComponentSpace.Inverse();

		TargetLocalSpace = &(Output.Pose.operator [](BoneIndex));
		FTransform TargetComponentSpace{ NoInit };
		// Out, child, parent
		FTransform::Multiply(&TargetComponentSpace, TargetLocalSpace, &ParentComponentSpace);

		return TargetComponentSpace;
	}

	FTransform ConvertComponentToLocal(FTransform& TargetComponentSpace)
	{
		// If it crashed here, you probably didn't call ConvertLocalToComponent first...
		*TargetLocalSpace = TargetComponentSpace.GetRelativeTransform(ParentComponentSpace);
		return *TargetLocalSpace;
	}

	FORCEINLINE bool operator<(const FBoneLookupTable& Other) const
	{
		return BoneIndex < Other.BoneIndex;
	}

	FORCEINLINE bool operator==(const FCompactPoseBoneIndex InIndex) const
	{
		return BoneIndex > -1 && BoneIndex == InIndex;
	}

	FORCEINLINE bool operator!=(const FCompactPoseBoneIndex InIndex) const
	{
		return BoneIndex == -1 || BoneIndex != InIndex;
	}
};


/**
 *	Handles the otherwise extremely expensive conversions "Local to Component" & "Component to Local" using a LUT at a mere fraction of the cost
 */
USTRUCT(BlueprintInternalUseOnly)
struct MOVEIT_API FAnimNode_LocalSkeletalControlBase : public FAnimNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink SourcePose;

public:
	/*
	* Max LOD that this node is allowed to run
	* For example if you have LODThreadhold to be 2, it will run until LOD 2 (based on 0 index)
	* when the component LOD becomes 3, it will stop update/evaluate
	* currently transition would be issue and that has to be re-visited
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta = (DisplayName = "LOD Threshold"))
	int32 LODThreshold;

	UPROPERTY(Transient)
	float BlendWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha)
	EAnimAlphaInputType AlphaInputType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha, meta = (PinShownByDefault, DisplayName = "bEnabled"))
	bool bAlphaBoolEnabled;

	// Current strength of the skeletal control
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha, meta = (PinShownByDefault))
	float Alpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha)
	FInputScaleBias AlphaScaleBias;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha, meta = (DisplayName = "Blend Settings"))
	FInputAlphaBoolBlend AlphaBoolBlend;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha, meta = (PinShownByDefault))
	FName AlphaCurveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Alpha)
	FInputScaleBiasClamp AlphaScaleBiasClamp;

public:
	FAnimNode_LocalSkeletalControlBase()
		: LODThreshold(INDEX_NONE)
		, BlendWeight(0.f)
		, AlphaInputType(EAnimAlphaInputType::Float)
		, bAlphaBoolEnabled(true)
		, Alpha(1.0f)
	{}

public:
#if WITH_EDITORONLY_DATA
	// forwarded pose data from the wired node which current node's skeletal control is not applied yet
	FCSPose<FCompactHeapPose> ForwardedPose;
#endif //#if WITH_EDITORONLY_DATA

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) final;
	virtual void Evaluate_AnyThread(FPoseContext& Output) final;
	virtual int32 GetLODThreshold() const override { return LODThreshold; }
	// End of FAnimNode_Base interface

protected:
	// Interface for derived skeletal controls to implement
	// use this function to update for skeletal control base
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) {}

	// Evaluate incoming pose.
	virtual void EvaluatePose_AnyThread(FPoseContext& Output);

	// Evaluate incoming pose.
	virtual void EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float InBlendWeight) {}

protected:
	// use this to retrieve the component space transform to modify
	FTransform ConvertLocalToComponent(FPoseContext& Output, const FCompactPoseBoneIndex BoneIndex);
	// Use this to convert the component space transform to local space without alpha blending
	FTransform ConvertComponentToLocal(FTransform& TargetComponentSpace, const FCompactPoseBoneIndex BoneIndex);
	// Use this to convert to local space and apply the transform with alpha blending
	void ApplyComponentToLocal(FPoseContext& Output, FTransform& TargetComponentSpace, const FCompactPoseBoneIndex BoneIndex, float Alpha);

	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) { return false; }

	// Use this to initialize any bone references you have
	void InitializeBoneParents(FBoneReference& BoneRef, const FBoneContainer& BoneContainer);

	// Use this to initialize any bone references you have
	void InitializeBoneParents(const FCompactPoseBoneIndex& BoneIndex, const FBoneContainer& BoneContainer);

	// Initialize any bone references you have here by passing them to InitializeBoneParents
	virtual void InitializeBoneParentCache(const FBoneContainer& BoneContainer) {};

	// Initialize any bone references you have
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) {};

	/** Allow base to add info to the node debug output */
	void AddDebugNodeData(FString& OutDebugData);

private:
	void InitializeBoneParentLUT(const FBoneContainer& BoneContainer);

	TArray<FBoneLookupTable> BoneLUT;
};