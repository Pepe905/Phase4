// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "MIBoneTools.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "MeshUtilities.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "PackageTools.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogBoneTools, Log, All);

#define LOCTEXT_NAMESPACE "MIBoneTools"


void UMIBoneTools::ErrorCantFindBone(USkeleton* Skeleton, const FName BoneName)
{
	const FString ErrorString = FString::Printf(TEXT("Bone does not exist { %s }"), *BoneName.ToString());
	const FText ErrorText = FText::FromString(ErrorString);

	FMessageLog Log = FMessageLog("AssetCheck");
	Log.Open(EMessageSeverity::Error, true);
	Log.Message(EMessageSeverity::Error, ErrorText);
}

void UMIBoneTools::SaveAndReloadAsset(UObject* Asset)
{
	// Dirty the package (needs saving)
	Asset->MarkPackageDirty();

	// Save the package
	FEditorFileUtils::PromptForCheckoutAndSave({ Asset->GetOutermost() }, false, false);

	// Reload the package with the new bones
	FText DummyText = FText::GetEmpty();
	UPackageTools::ReloadPackages({ Asset->GetOutermost() }, DummyText, UPackageTools::EReloadPackagesInteractionMode::AssumePositive);
}

void UMIBoneTools::ForceCloseAsset(UObject* Asset)
{
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	AssetEditorSubsystem->CloseAllEditorsForAsset(Asset);
}

TArray<USkeletalMesh*> UMIBoneTools::FindCompatibleMeshes(USkeleton* const Skeleton) const
{
	FARFilter Filter;
	Filter.ClassNames.Add(USkeletalMesh::StaticClass()->GetFName());

	FString SkeletonString = FAssetData(Skeleton).GetExportTextName();
	PRAGMA_DISABLE_DEPRECATION_WARNINGS  // UE5
	Filter.TagsAndValues.Add(GET_MEMBER_NAME_CHECKED(USkeletalMesh, Skeleton), SkeletonString);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS  // UE5

	TArray<FAssetData> AssetList;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);

	TArray<USkeletalMesh*> Result;

	if (AssetList.Num() > 0)
	{
		for (const FAssetData& AssetDataList : AssetList)
		{
			if (USkeletalMesh* const SkelMesh = Cast<USkeletalMesh>(AssetDataList.GetAsset()))
			{
				Result.Add(SkelMesh);
			}
		}
	}

	return Result;
}

void UMIBoneTools::UpdateBoneNameForIK(FMeshBoneInfo& BoneInfo, const FString& IKPrefix, const FString& IKSuffix)
{
	BoneInfo.ExportName = IKPrefix + BoneInfo.Name.ToString() + IKSuffix;
	BoneInfo.Name = FName(*BoneInfo.ExportName);
}

bool UMIBoneTools::FindBone(const TArray<FMeshBoneInfo>& BoneInfos, const TArray<FTransform>& BoneTMs, USkeleton* Skel, const FName& BoneName, FMeshBoneInfo& OutBoneInfo, FTransform& OutBoneTM, int32& OutBoneIndex)
{
	for (int32 i = 0; i < BoneInfos.Num(); i++)
	{
		const FMeshBoneInfo& BoneInfo = BoneInfos[i];
		if (BoneInfo.Name == BoneName)
		{
			OutBoneInfo = BoneInfo;
			OutBoneIndex = i;

			// Correct transforms would be nice but not needed - they just sit at 0,0,0 and the anim graph copies them to FK counterparts at runtime anyway
			//OutBoneTM = BoneTMs[i];

			return true;
		}
	}

	ErrorCantFindBone(Skel, BoneName);
	return false;
}

void UMIBoneTools::AddBoneToMesh(USkeletalMesh* const Mesh, int32 LODIndex, int32 BoneIndex)
{
	Mesh->GetImportedModel()->LODModels[LODIndex].RequiredBones.AddUnique(BoneIndex);
	Mesh->GetImportedModel()->LODModels[LODIndex].ActiveBoneIndices.AddUnique(BoneIndex);

	for (int32 i = 0; i < Mesh->GetImportedModel()->LODModels[LODIndex].Sections.Num(); i++)
	{
		Mesh->GetImportedModel()->LODModels[LODIndex].Sections[i].BoneMap.AddUnique(BoneIndex);
	}
}

void UMIBoneTools::AddBoneToSkeleton(FMIBoneInfo& BoneInfo, TArray<USkeletalMesh*> Meshes)
{
	for (USkeletalMesh* const SkelMesh : Meshes)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS  // UE5
		if (SkelMesh->RefSkeleton.FindRawBoneIndex(BoneInfo.BoneInfo.Name) == INDEX_NONE)
		{
			FReferenceSkeletonModifier Mod = FReferenceSkeletonModifier(SkelMesh->RefSkeleton, SkelMesh->Skeleton);
			Mod.Add(FMeshBoneInfo(BoneInfo.BoneInfo.Name, BoneInfo.BoneInfo.ExportName, BoneInfo.BoneInfo.ParentIndex), BoneInfo.TM);

			BoneInfo.Index = SkelMesh->RefSkeleton.FindRawBoneIndex(BoneInfo.BoneInfo.Name);
		}
		PRAGMA_ENABLE_DEPRECATION_WARNINGS  // UE5
	}
}

void UMIBoneTools::AddIKBones(
	UObject* SkeletalObject, 
	const FName& LeftFootName /* = TEXT("foot_l") */, 
	const FName& RightFootName /* = TEXT("foot_r") */, 
	const FName& LeftHandName /* = TEXT("hand_l") */, 
	const FName& RightHandName /* = TEXT("hand_r") */, 
	const FName& IKPrefixName /* = TEXT("ik_") */, 
	const FName& IKSuffixName /* = TEXT("") */
)
{
	// IK name would be same as FK name, bone names should be unique, exit
	if (IKPrefixName.IsNone() && IKSuffixName.IsNone())
	{
		const FString ErrorString = "Both IK Prefix and Suffix are empty; IK name would be same as FK name. Bone names should be unique. Aborting.";
		const FText ErrorText = FText::FromString(ErrorString);

		FMessageLog Log = FMessageLog("AssetCheck");
		Log.Open(EMessageSeverity::Error, true);
		Log.Message(EMessageSeverity::Error, ErrorText);

		return;
	}

	// Currently only supports regular limbed characters and creatures, perhaps in the future you'll be able to leave out bones
	if (LeftFootName.IsNone() || RightFootName.IsNone() || LeftHandName.IsNone() || RightHandName.IsNone())
	{
		const FString ErrorString = "All bone names should be filled in. In the future characters or creatures missing limbs may be supported but not now. Aborting.";
		const FText ErrorText = FText::FromString(ErrorString);

		FMessageLog Log = FMessageLog("AssetCheck");
		Log.Open(EMessageSeverity::Error, true);
		Log.Message(EMessageSeverity::Error, ErrorText);

		return;
	}

	// Determine correct skeleton to use
	PRAGMA_DISABLE_DEPRECATION_WARNINGS  // UE5
	USkeleton* const FromSkeleton = Cast<USkeleton>(SkeletalObject);

	USkeletalMesh* const FromMesh = Cast<USkeletalMesh>(SkeletalObject);

	const bool bSkeletonBased = FromSkeleton != nullptr;
	USkeleton* const Skeleton = (bSkeletonBased) ? FromSkeleton : FromMesh->Skeleton;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS  // UE5

	// Close the skeleton (don't want issues)
	ForceCloseAsset(SkeletalObject);

	if (Skeleton)
	{
		// Refs
		const FReferenceSkeleton& RefSkeletonConst = Skeleton->GetReferenceSkeleton();
		const TArray<FMeshBoneInfo>& BoneInfos = RefSkeletonConst.GetRawRefBoneInfo();
		const TArray<FTransform>& BoneTMs = RefSkeletonConst.GetRefBonePose();

		// Find each corresponding FK bone
		FMIBoneInfo LeftFoot = FMIBoneInfo();
		FMIBoneInfo RightFoot = FMIBoneInfo();
		FMIBoneInfo LeftHand = FMIBoneInfo();
		FMIBoneInfo RightHand = FMIBoneInfo();

		bool bFoundAllBones = true;
		bFoundAllBones &= FindBone(BoneInfos, BoneTMs, Skeleton, LeftFootName, LeftFoot.BoneInfo, LeftFoot.TM, LeftFoot.Index);
		bFoundAllBones &= FindBone(BoneInfos, BoneTMs, Skeleton, RightFootName, RightFoot.BoneInfo, RightFoot.TM, RightFoot.Index);
		bFoundAllBones &= FindBone(BoneInfos, BoneTMs, Skeleton, LeftHandName, LeftHand.BoneInfo, LeftHand.TM, LeftHand.Index);
		bFoundAllBones &= FindBone(BoneInfos, BoneTMs, Skeleton, RightHandName, RightHand.BoneInfo, RightHand.TM, RightHand.Index);

		if (!bFoundAllBones)
		{
			// Didn't find all required bones, exit
			return;
		}

		// Get all meshes compatible with skeleton (to add bones to their LODs)
		TArray<USkeletalMesh*> Meshes = !bSkeletonBased ? TArray<USkeletalMesh*>{FromMesh} : FindCompatibleMeshes(Skeleton);

		// Close any open meshes
		for (USkeletalMesh* const Mesh : Meshes)
		{
			ForceCloseAsset(Mesh);
		}

		// Add and cache root & parent (gun) bones
		const FName IKFootRootName = TEXT("ik_foot_root");
		const FName IKHandRootName = TEXT("ik_hand_root");
		const FName IKHandGunName = TEXT("ik_hand_gun");

		FMIBoneInfo IKFootRoot = FMIBoneInfo(FMeshBoneInfo(IKFootRootName, IKFootRootName.ToString(), 0), FTransform::Identity, -1);
		FMIBoneInfo IKHandRoot = FMIBoneInfo(FMeshBoneInfo(IKHandRootName, IKHandRootName.ToString(), 0), FTransform::Identity, -1);

		AddBoneToSkeleton(IKFootRoot, Meshes);
		AddBoneToSkeleton(IKHandRoot, Meshes);

		FMIBoneInfo IKHandGun = FMIBoneInfo(FMeshBoneInfo(IKHandGunName, IKHandGunName.ToString(), IKHandRoot.Index), RightHand.TM, -1);
		AddBoneToSkeleton(IKHandGun, Meshes);

		// Update parent indices based on newly added root bones
		LeftFoot.BoneInfo.ParentIndex = IKFootRoot.Index;
		RightFoot.BoneInfo.ParentIndex = IKFootRoot.Index;
		LeftHand.BoneInfo.ParentIndex = IKHandGun.Index;
		RightHand.BoneInfo.ParentIndex = IKHandGun.Index;

		// Create an array of bones to process at once
		TArray<FMIBoneInfo> NewBoneInfo = { LeftFoot, RightFoot, LeftHand, RightHand };

		// Add all new child IK bones
		for (FMIBoneInfo& BI : NewBoneInfo)
		{
			const FString IKPrefix = IKPrefixName.IsNone() ? FString() : IKPrefixName.ToString();
			const FString IKSuffix = IKSuffixName.IsNone() ? FString() : IKSuffixName.ToString();
			UpdateBoneNameForIK(BI.BoneInfo, IKPrefix, IKSuffix);

			AddBoneToSkeleton(BI, Meshes);
		}

		IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");

		// Add bones to every LOD of every compatible mesh
		for (USkeletalMesh* const Mesh : Meshes)
		{
			for (int32 i = 0; i < Mesh->GetImportedModel()->LODModels.Num(); i++)
			{
				AddBoneToMesh(Mesh, i, IKFootRoot.Index);
				AddBoneToMesh(Mesh, i, IKHandRoot.Index);
				AddBoneToMesh(Mesh, i, IKHandGun.Index);

				for (FMIBoneInfo& BI : NewBoneInfo)
				{
					AddBoneToMesh(Mesh, i, BI.Index);
				}

				// Reload mesh LODs
				Mesh->AddBoneToReductionSetting(i, TEXT(""));
				MeshUtilities.RemoveBonesFromMesh(Mesh, i, nullptr);
			}

			// Add all new bones from the mesh to the skeleton then save and reload it (reloading it refreshes bones)
			Skeleton->MergeAllBonesToBoneTree(Mesh);
			SaveAndReloadAsset(Mesh);
		}

		// Add all new bones from the mesh to the skeleton then save and reload it (reloading it refreshes bones)
		SaveAndReloadAsset(Skeleton);
	}
}

#undef LOCTEXT_NAMESPACE