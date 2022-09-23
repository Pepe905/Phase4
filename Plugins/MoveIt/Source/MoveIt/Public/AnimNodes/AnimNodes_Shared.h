// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "BoneContainer.h"
#include "SceneManagement.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes_Shared.generated.h"

enum class EMIJumpState : uint8
{
	JS_None,
	JS_Ground,
	JS_Jump,
	JS_Fall,
	JS_Fly,
	JS_Swim,
	JS_MAX = 255
};

USTRUCT(BlueprintType)
struct FMICopyIKBones_Bone
{
	GENERATED_BODY()

	/** Source bone to copy from */
	UPROPERTY(EditAnywhere, Category = "Copy IK Bones")
	FBoneReference Bone;

	/** Target bone to copy over */
	UPROPERTY(EditAnywhere, Category = "Copy IK Bones")
	FBoneReference IK;

	FCompactPoseBoneIndex BoneIndex;
	FCompactPoseBoneIndex IKIndex;

	FMICopyIKBones_Bone()
		: BoneIndex(-1)
		, IKIndex(-1)
	{}

	FMICopyIKBones_Bone(const FBoneReference& InBone, const FBoneReference& InIK)
		: Bone(InBone)
		, IK(InIK)
		, BoneIndex(-1)
		, IKIndex(-1)
	{}
};

struct FAnimNodeStatics
{
	FAnimNodeStatics() {}

	static void DrawDebugFootBox(FPrimitiveDrawInterface* PDI, const FVector& Center, const FVector& Box, const FQuat& Rotation, const FColor& Color, float Thickness = 0.5f, ESceneDepthPriorityGroup DepthPriority = SDPG_World);
	static void DrawDebugFootBox(FAnimInstanceProxy* Proxy, const FVector& Center, const FVector& Box, const FQuat& Rotation, const FColor& Color, float Thickness = 0.5f);

	static void DrawDebugLocator(FPrimitiveDrawInterface* PDI, const FVector& Center, const FQuat& Rotation, float LocatorSize, const FColor& Color, float Thickness = 0.5f, ESceneDepthPriorityGroup DepthPriority = SDPG_World);
	static void DrawDebugLocator(FAnimInstanceProxy* Proxy, const FVector& Center, const FQuat& Rotation, float LocatorSize, const FColor& Color, float Thickness = 0.5f);

	static void DrawDebugLine(FPrimitiveDrawInterface* PDI, const FVector& Start, const FVector& End, const FColor& Color, float Thickness = 0.5f, ESceneDepthPriorityGroup DepthPriority = SDPG_World);
	static void DrawDebugLine(FAnimInstanceProxy* Proxy, const FVector& Start, const FVector& End, const FColor& Color, float Thickness = 0.5f);
};