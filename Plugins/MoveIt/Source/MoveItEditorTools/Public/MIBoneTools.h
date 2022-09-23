// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blutility/Classes/EditorUtilityWidget.h"
#include "MIBoneTools.generated.h"

struct FMeshBoneInfo;

struct FMIBoneInfo
{
	FMIBoneInfo(const FMeshBoneInfo& InBoneInfo, const FTransform& InTM, int32 InIndex)
		: BoneInfo(InBoneInfo)
		, TM(InTM)
		, Index(InIndex)
	{}

	FMIBoneInfo()
		: BoneInfo(FMeshBoneInfo())
		, TM(FTransform::Identity)
		, Index(-1)
	{}

	FMeshBoneInfo BoneInfo;
	FTransform TM;
	int32 Index;
};

/**
 *
 */
UCLASS()
class MOVEITEDITORTOOLS_API UMIBoneTools : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	/** Named a bone wrong in most cases */
	void ErrorCantFindBone(USkeleton* Skeleton, const FName BoneName);

	/** Reloading the asset will force it to refresh changes to bones */
	void SaveAndReloadAsset(UObject* Asset);

	/** Closing open editors prevents issues from forming */
	void ForceCloseAsset(UObject* Asset);

	/** @return Every mesh compatible with the given skeleton */
	TArray<USkeletalMesh*> FindCompatibleMeshes(USkeleton* const Skeleton) const;

	/** Applies IK prefix and suffix */
	void UpdateBoneNameForIK(FMeshBoneInfo& BoneInfo, const FString& IKPrefix, const FString& IKSuffix);

	bool FindBone(const TArray<FMeshBoneInfo>& BoneInfos, const TArray<FTransform>& BoneTMs, USkeleton* Skel, const FName& BoneName, FMeshBoneInfo& OutBoneInfo, FTransform& OutBoneTM, int32& OutBoneIndex);

	void AddBoneToMesh(USkeletalMesh* const Mesh, int32 LODIndex, int32 BoneIndex);

	void AddBoneToSkeleton(FMIBoneInfo& BoneInfo, TArray<USkeletalMesh*> Meshes);

	/**
	 * @param SkeletalObject: Can be either a skeleton (will update all compatible meshes) or a single mesh
	 */
	UFUNCTION(BlueprintCallable, Category = "Development|Editor")
	void AddIKBones(UObject* SkeletalObject, const FName& LeftFootName = TEXT("foot_l"), const FName& RightFootName = TEXT("foot_r"), const FName& LeftHandName = TEXT("hand_l"), const FName& RightHandName = TEXT("hand_r"), const FName& IKPrefixName = TEXT("ik_"), const FName& IKSuffixName = TEXT(""));
};
