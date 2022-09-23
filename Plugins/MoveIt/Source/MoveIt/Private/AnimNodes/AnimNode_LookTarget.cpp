// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_LookTarget.h"
#include "Animation/AnimInstanceProxy.h"


void FAnimNode_LookTarget::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_LocalSkeletalControlBase::Initialize_AnyThread(Context);

	bReinitialize = true;
}

void FAnimNode_LookTarget::ResetDynamics(ETeleportType InTeleportType)
{
	FAnimNode_LocalSkeletalControlBase::ResetDynamics(InTeleportType);

	bReinitialize = true;
}

void FAnimNode_LookTarget::UpdateInternal(const FAnimationUpdateContext& Context)
{
	RemainingTime += Context.GetDeltaTime();

	// Fixed step simulation at 120hz
	FixedTimeStep = (1.f / SolverFrequency) * TimeDilation;
}

void FAnimNode_LookTarget::EvaluateLocalSkeletalControl_AnyThread(FPoseContext& Output, float ActualBlendWeight)
{
	// Cache vars for use
	const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();

	USkeletalMeshComponent* SkelMeshComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const FTransform& ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();

	if (TargetInputType == EMILookTargetInput::LTI_Actor && !IsValid(Target))
	{
		return;
	}

	if (!SkelMeshComp || !SkelMeshComp->GetOwner())
	{
		return;
	}

	if (!SkelMeshComp->DoesSocketExist(EyeLevelSocket))
	{
		UE_LOG(LogTemp, Warning, TEXT("Socket { %s } does not exist on { %s }. LookTarget can not continue."), *EyeLevelSocket.ToString(), *GetNameSafe(SkelMeshComp->SkeletalMesh));
		return;
	}

	FTransform EyeTM = SkelMeshComp->GetSocketTransform(EyeLevelSocket);

	const FVector OwnerLocation = ComponentTransform.GetLocation();
	const FVector TargetLocation = (TargetInputType == EMILookTargetInput::LTI_Location) ? Location : Target->GetActorLocation();

	if (!SkelMeshComp->GetWorld()) { return; }
	if (!SkelMeshComp->GetWorld()->IsGameWorld() && !bWorkOutsidePIE)
	{
		return;
	}

	// ------ COMPUTE COMPONENT DIRECTION -------
	// ------------------------------------------

	const FVector WorldDirection = TargetLocation - EyeTM.GetLocation();

	// Compute Component Direction
	constexpr float MinDistance{ 0.5f };
	constexpr float MinDistanceSquared{ MinDistance * MinDistance };

	const double SizeSquared{ WorldDirection.SizeSquared() };

	if (!(SizeSquared >= MinDistanceSquared))
	{
		return;
	}

	// normal in same world direction.
	const FVector TargetComponentDirection = ComponentTransform.InverseTransformVectorNoScale(WorldDirection) / FMath::Sqrt(SizeSquared);

	// ----------- SPRING ALGORITHM -------------
	// ------------------------------------------
	if (bReinitialize)
	{
		// Don't interpolate old values
		bReinitialize = false;

		for (FMILookTargetBone& Bone : Bones)
		{
			Bone.OldComponentDirection = TargetComponentDirection;
			Bone.ComponentDirection = TargetComponentDirection;
			Bone.BoneSpring.Driver.bRunning = false;
			Bone.BoneSpring.bRunning = false;
			Bone.BoneSpring.ReadInput(TargetComponentDirection, FixedTimeStep, SpringDamping, SpringStrength, SpringStiffness);
		}
	}
	else
	{
		if (!FMath::IsNearlyZero(FixedTimeStep, KINDA_SMALL_NUMBER))
		{

			// count steps required.
			int32 step_count{ 0 };

			while (RemainingTime > FixedTimeStep)
			{
				RemainingTime -= FixedTimeStep;
				step_count++;
			}

			// when steps are necessary..
			if (step_count != 0)
			{
				// if its not one then there is at least a single substep.
				if (step_count != 1)
				{
					// do one substep but collect initial data.
					for (FMILookTargetBone& Bone : Bones)
					{
						Bone.ComponentDirection = TargetComponentDirection;

						// get rate in frames. (instead of seconds)
						FAngularVector const AngularOverSteps{ FAngularVector::BetweenNormals, Bone.OldComponentDirection, Bone.ComponentDirection, (float)step_count };

						// get rotation in one frame.
						Bone.TempAdvanceRot = AngularOverSteps.ToQuat();

						// rotate the old by temp advance rot.
						Bone.OldComponentDirection = Bone.TempAdvanceRot.RotateVector(Bone.OldComponentDirection);

						// and substep once.
						Bone.BoneSpring.ReadInput(Bone.OldComponentDirection, FixedTimeStep, SpringDamping, SpringStrength, SpringStiffness);
					}

					// now more sub steps.
					for (int pre_step{ 2 }; pre_step != step_count; ++pre_step)
					{
						for (FMILookTargetBone& Bone : Bones)
						{
							// rotate the old by temp advance rot.
							Bone.OldComponentDirection = Bone.TempAdvanceRot.RotateVector(Bone.OldComponentDirection);

							// and substep once.
							Bone.BoneSpring.ReadInput(Bone.OldComponentDirection, FixedTimeStep, SpringDamping, SpringStrength, SpringStiffness);
						}
					}
				}

				// actual (final) step
				for (FMILookTargetBone& Bone : Bones)
				{
					// step to real target.
					Bone.BoneSpring.ReadInput(Bone.ComponentDirection, FixedTimeStep, SpringDamping, SpringStrength, SpringStiffness);

					// store old target.
					Bone.OldComponentDirection = Bone.ComponentDirection;
				}
			}
		}
	}

	// ------------ TRANSFORM BONES -------------
	// Rotate the bones to look at the target
	// ------------------------------------------

	for (FMILookTargetBone& Bone : Bones)
	{
		FTransform BoneTM = ConvertLocalToComponent(Output, Bone.Index);
		PointBoneAt(Bone.BoneSpring.Position, BoneTM, ComponentTransform);
		ApplyComponentToLocal(Output, BoneTM, Bone.Index, Bone.Bias * ActualBlendWeight);
	}
}

bool FAnimNode_LookTarget::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	for (const FMILookTargetBone& Bone : Bones)
	{
		if (!Bone.Bone.IsValidToEvaluate(RequiredBones)) { return false; }
	}

	return true;
}

void FAnimNode_LookTarget::InitializeBoneParentCache(const FBoneContainer& BoneContainer)
{
	for (FMILookTargetBone& Bone : Bones)
	{
		InitializeBoneParents(Bone.Bone, BoneContainer);
	}
}

void FAnimNode_LookTarget::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	float Lowest = 9999.f;
	float Sum = 0.f;

	for (FMILookTargetBone& Bone : Bones)
	{
		Bone.Bone.Initialize(RequiredBones);
		Bone.Index = Bone.Bone.GetCompactPoseIndex(RequiredBones);
		Bone.Bias = Bone.Weight;

		// Find lowest weight
		if (Bone.Bias < Lowest)
		{
			Lowest = Bone.Bias;
		}
	}

	// Divide by lowest and compute sum
	for (FMILookTargetBone& Bone : Bones)
	{
		Bone.Bias /= Lowest;
		Sum += Bone.Bias;
	}

	// Scale array to become 1.0
	for (FMILookTargetBone& Bone : Bones)
	{
		Bone.Bias /= Sum;
	}
}
