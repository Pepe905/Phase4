// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_Gait.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/BlendProfile.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/WorldSettings.h"


void FAnimNode_Gait::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_LocalSkeletalControlBase::Initialize_AnyThread(Context);

	RemainingTime = 0.f;
}

void FAnimNode_Gait::PreUpdate(const UAnimInstance* InAnimInstance)
{
	// Copied from FAnimNode_SpringBone::PreUpdate

	const USkeletalMeshComponent* SkelComp = InAnimInstance->GetSkelMeshComponent();
	const UWorld* World = SkelComp->GetWorld();
	check(World->GetWorldSettings());
	TimeDilation = World->GetWorldSettings()->GetEffectiveTimeDilation();

	OwnerVelocity = (SkelComp->GetOwner()) ? SkelComp->GetOwner()->GetVelocity() : FVector::ZeroVector;
}

void FAnimNode_Gait::UpdateInternal(const FAnimationUpdateContext& Context)
{
	RemainingTime += Context.GetDeltaTime();

	// Fixed step simulation at 120hz
	FixedTimeStep = (1.f / 120.f) * TimeDilation;
}

void FAnimNode_Gait::EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float ActualBlendWeight)
{
	// Cache vars for use
	const UWorld* const World = Output.AnimInstanceProxy->GetAnimInstanceObject()->GetWorld();

	const USkeletalMeshComponent* SkelComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const FTransform& MeshTM = Output.AnimInstanceProxy->GetComponentTransform();

	const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();

	const FCompactPoseBoneIndex IKRootBoneIndex = IKRoot.GetCompactPoseIndex(BoneContainer);
	const FCompactPoseBoneIndex PelvisBoneIndex = Pelvis.GetCompactPoseIndex(BoneContainer);

	// Adjust gait based on slope
	float SlopeGaitMultiplier = CurrentSlopeGaitMultiplier;
	if (bAdjustGaitToSlope)
	{
		if (SlopeTraceAheadTraceCount > 0 && OwnerVelocity.Size() > 1.f && SkelComp->GetOwner())
		{
			TArray<FHitResult> Hits;
			const float TraceSubdivideOffset = SlopeTraceAheadDist / SlopeTraceAheadTraceCount;

			FVector TraceStart = FVector::ZeroVector;

			for (float i = 0.f; i < (SlopeTraceAheadDist - 0.1f); i += TraceSubdivideOffset)  // Note: 0.1f is error tolerance
			{
				TraceStart = MeshTM.GetLocation() + FVector(0.f, 0.f, SlopeTraceStartHeightAboveMesh) + OwnerVelocity.GetSafeNormal() * i;
				const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, SlopeTraceStartHeightAboveMesh + SlopeTraceLengthBelowMesh);
				//DrawDebugPoint(World, TraceStart, 16.f, FColor::White);
				FHitResult Hit(ForceInit);
				UKismetSystemLibrary::LineTraceSingle(
					SkelComp->GetOwner(),
					TraceStart,
					TraceEnd,
					SlopeTraceType(),
					true,
					TArray<AActor*>() = { SkelComp->GetOwner() },
					EDrawDebugTrace::None,
					Hit,
					false
				);

				if (Hit.bBlockingHit && !Hit.bStartPenetrating)
				{
					Hits.Add(Hit);
				}
			}

			if (Hits.Num() > 0)
			{
				float HitNormal = 0.f;
				for (const FHitResult& Hit : Hits)
				{
					HitNormal += (Hit.Normal | FVector::UpVector);
				}
				HitNormal /= Hits.Num();
				HitNormal = FMath::RadiansToDegrees(FMath::Acos(HitNormal));

				SlopeGaitMultiplier = SlopeAngleGaitMultiplierCurve.GetRichCurveConst()->Eval(HitNormal, 1.f);
			}
		}
		CurrentSlopeGaitMultiplier = FMath::FInterpConstantTo(CurrentSlopeGaitMultiplier, SlopeGaitMultiplier, Output.AnimInstanceProxy->GetDeltaSeconds(), SlopeSmoothingRate);
	}
	else
	{
		SlopeGaitMultiplier = 1.f;
	}

	// Calculate locations for every foot
	for (FMIGait_Foot& Foot : Feet)
	{
		const FCompactPoseBoneIndex BoneIndex = Foot.Foot.GetCompactPoseIndex(BoneContainer);
		const FCompactPoseBoneIndex IKBoneIndex = Foot.IKFoot.GetCompactPoseIndex(BoneContainer);
		const FCompactPoseBoneIndex PoleBoneIndex = Foot.Pole.GetCompactPoseIndex(BoneContainer);
		const FCompactPoseBoneIndex IKPoleBoneIndex = Foot.IKPole.GetCompactPoseIndex(BoneContainer);
		const FCompactPoseBoneIndex ParentBoneIndex = Foot.Parent.GetCompactPoseIndex(BoneContainer);

		// Cache bone transforms
		FTransform BoneTM = ConvertLocalToComponent(Output, BoneIndex);
		FTransform IKBoneTM = ConvertLocalToComponent(Output, IKBoneIndex);
		FTransform PoleBoneTM = ConvertLocalToComponent(Output, PoleBoneIndex);
		FTransform IKPoleBoneTM = ConvertLocalToComponent(Output, IKPoleBoneIndex);
		FTransform ParentBoneTM = ConvertLocalToComponent(Output, ParentBoneIndex);

		// Cache frequently used transform translations
		const FVector ParentLocation = ParentBoneTM.GetTranslation();
		const FVector IKStartLocation = IKBoneTM.GetTranslation();

		// Compute the length of each limb
		float PoleBoneLength = (BoneTM.GetLocation() - PoleBoneTM.GetLocation()).Size();
		float ParentBoneLength = (PoleBoneTM.GetLocation() - ParentBoneTM.GetLocation()).Size();

		const float CurrentGait = FMath::Clamp<float>(Gait * CurrentSlopeGaitMultiplier * GaitMultiplier * 0.98f, MinGait, MaxGait);  // 2% error threshold

		// Get scale pivot
		const FVector IKPivotLocation = FVector(ParentLocation.X, ParentLocation.Y, IKStartLocation.Z);

		// Compute IK Location
		const FVector IKLocation = ((IKStartLocation - IKPivotLocation) * CurrentGait) + IKPivotLocation;

		// Use the computed bone lengths to clamp the IK position if desired
		FVector TargetIKLocation = IKLocation;
		if (bClampIKLengthToBoneLength)
		{
			const float IKLength = (IKStartLocation - ParentLocation).Size();
			const float TargetIKLength = (IKLocation - ParentLocation).Size();
			const float LengthMultiplier = 1.f - (1.f / CurrentGait);
			const float TotalLength = PoleBoneLength + ParentBoneLength;
			const float MaxScaledLength = IKLength + (TotalLength - IKLength) * LengthMultiplier;
			const FVector IKDirection = (IKLocation - ParentLocation).GetSafeNormal();
			if (TargetIKLength > IKLength)
			{
				const FVector Adjusted = IKLocation + IKDirection * (MaxScaledLength - TargetIKLength);
				TargetIKLocation = (Adjusted - ParentLocation).Size() > (TargetIKLocation - ParentLocation).Size() ? TargetIKLocation : Adjusted;
			}
		}
		
		if (CurrentGait < MaxSpeedFootHeightBias)
		{
			const float FootZMultiplier = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(MinSpeedFootHeightBias, MaxSpeedFootHeightBias), CurrentGait);
			TargetIKLocation.Z = (IKLocation - (IKLocation - ParentLocation).GetSafeNormal()).Z * FootZMultiplier;
		}

		// Set the final position of the IK foot
		IKBoneTM.SetTranslation(TargetIKLocation);
		ApplyComponentToLocal(Output, IKBoneTM, IKBoneIndex, ActualBlendWeight);

		// Store results with the corresponding foot
		Foot.LimbRootLocation = ParentLocation;
		Foot.OriginLocation = IKStartLocation;
		Foot.ActualLocation = TargetIKLocation;
	}

	if (PelvisDisplacement != 0.f)
	{
		// Reset velocity values each frame
		if (RemainingTime == 0.f)
		{
			for (FMIGait_Foot& Foot : Feet)
			{
				Foot.BoneLocation = Foot.OriginLocation;
				Foot.BoneVelocity = FVector::ZeroVector;
			}
		}

		if (!FMath::IsNearlyZero(FixedTimeStep, KINDA_SMALL_NUMBER))
		{
			while (RemainingTime > FixedTimeStep)
			{
				for (FMIGait_Foot& Foot : Feet)
				{
					// Based largely on FAnimNode_SpringBone

					// Calculate error vector.
					const FVector IKDirection = (Foot.ActualLocation - Foot.LimbRootLocation).GetSafeNormal();
					const float IKStretch = ((Foot.ActualLocation - Foot.LimbRootLocation).Size() - (Foot.OriginLocation - Foot.LimbRootLocation).Size());
					const FVector Error = IKDirection * IKStretch;
					const FVector DampingForce = PelvisDamping * Foot.BoneVelocity;
					const FVector SpringForce = PelvisTightness * Error;

					// Calculate force based on error and vel
					const FVector Acceleration = SpringForce - DampingForce;

					// Integrate velocity
					// Make sure damping with variable frame rate actually dampens velocity. Otherwise Spring will go nuts.
					float const CutOffDampingValue = 1.f / FixedTimeStep;
					if (PelvisDamping > CutOffDampingValue)
					{
						const float SafetyScale = CutOffDampingValue / PelvisDamping;
						Foot.BoneVelocity += SafetyScale * (Acceleration * FixedTimeStep);
					}
					else
					{
						Foot.BoneVelocity += (Acceleration * FixedTimeStep);
					}

					// Integrate position
					const FVector OldBoneLocation = Foot.BoneLocation;
					const FVector DeltaMove = (Foot.BoneVelocity * FixedTimeStep);
					Foot.BoneLocation += DeltaMove;

					// Update velocity to reflect post processing done to bone location.
					Foot.BoneVelocity = (Foot.BoneLocation - OldBoneLocation) / FixedTimeStep;

					check(!Foot.BoneLocation.ContainsNaN());
					check(!Foot.BoneVelocity.ContainsNaN());
				}

				RemainingTime -= FixedTimeStep;
			}
		}

		// Add each bone velocity to get the overall spring force
		FVector SpringForce = FVector::ZeroVector;
		for (const FMIGait_Foot& Foot : Feet)
		{
			SpringForce += Foot.BoneVelocity;
		}

		// Compute pelvis location
		FTransform PelvisBoneTM = ConvertLocalToComponent(Output, PelvisBoneIndex);
		const FVector PelvisOriginLocation = PelvisBoneTM.GetLocation();
		const FVector PelvisNewLocation = PelvisOriginLocation + SpringForce;
		const FVector PelvisTargetLocation = PelvisOriginLocation + (SpringForce * PelvisDisplacement);

		// Apply pelvis transform
		PelvisBoneTM.SetTranslation(PelvisTargetLocation);
		ApplyComponentToLocal(Output, PelvisBoneTM, PelvisBoneIndex, ActualBlendWeight);
	}
}

bool FAnimNode_Gait::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	for (FMIGait_Foot& Foot : Feet)
	{
		if (!Foot.Foot.IsValidToEvaluate(RequiredBones)) { return false; }
		if (!Foot.IKFoot.IsValidToEvaluate(RequiredBones)) { return false; }
		if (!Foot.Pole.IsValidToEvaluate(RequiredBones)) { return false; }
		if (!Foot.IKPole.IsValidToEvaluate(RequiredBones)) { return false; }
		if (!Foot.Parent.IsValidToEvaluate(RequiredBones)) { return false; }
	}

	return IKRoot.IsValidToEvaluate(RequiredBones) && Pelvis.IsValidToEvaluate(RequiredBones);
}

void FAnimNode_Gait::InitializeBoneParentCache(const FBoneContainer& BoneContainer)
{
	InitializeBoneParents(IKRoot, BoneContainer);
	InitializeBoneParents(Pelvis, BoneContainer);
	for (FMIGait_Foot& IKFoot : Feet)
	{
		InitializeBoneParents(IKFoot.Foot, BoneContainer);
		InitializeBoneParents(IKFoot.IKFoot, BoneContainer);
		InitializeBoneParents(IKFoot.Pole, BoneContainer);
		InitializeBoneParents(IKFoot.IKPole, BoneContainer);
		InitializeBoneParents(IKFoot.Parent, BoneContainer);
	}
}

void FAnimNode_Gait::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	IKRoot.Initialize(RequiredBones);
	Pelvis.Initialize(RequiredBones);
	for (FMIGait_Foot& IKFoot : Feet)
	{
		IKFoot.Foot.Initialize(RequiredBones);
		IKFoot.IKFoot.Initialize(RequiredBones);
		IKFoot.Pole.Initialize(RequiredBones);
		IKFoot.IKPole.Initialize(RequiredBones);
		IKFoot.Parent.Initialize(RequiredBones);
	}
}
