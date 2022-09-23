// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_LandingPose.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "MITypes.h"


void FAnimNode_LandingPose::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_AdditiveBlendSpace::Initialize_AnyThread(Context);

	const USkeletalMeshComponent* const SkelComp = Context.AnimInstanceProxy->GetSkelMeshComponent();
	Character = (SkelComp && SkelComp->GetOwner()) ? Cast<ACharacter>(SkelComp->GetOwner()) : nullptr;
}

void FAnimNode_LandingPose::PreUpdate(const UAnimInstance* InAnimInstance)
{
	const USkeletalMeshComponent* const SkelComp = InAnimInstance->GetSkelMeshComponent();

	if (!SkelComp || !Character || !Character->GetCharacterMovement() || !Character->GetWorld())
	{
		return;
	}

	if (!Character->GetWorld()->IsGameWorld() && !bWorkOutsidePIE) { return; }

	OwnerVelocity = Character->GetVelocity();

	bool bCanLand = Character->GetCharacterMovement()->IsFalling() && Character->GetVelocity().Z < JumpStartZVelocity;

	// Predict the landing location
	bCanLand &= PredictLandingLocation(PredictedLanding);

	FHitResult& Hit = PredictedLanding.HitResult;

	// Landing location is walkable
	bCanLand &= Character->GetCharacterMovement()->IsWalkable(Hit);

	// Ensure we are close enough to the ground
	const float DistToGround = Character->GetActorLocation().Z - Hit.Location.Z;
	if (bCanLand)
	{
		bCanLand &= DistToGround <= StartMinDistFromGround;
	}

	bEnableLanding = bCanLand;
}

void FAnimNode_LandingPose::UpdateBlendSpace(const FAnimationUpdateContext& Context)
{
	if (!bEnableLanding)
	{
		X = 0.f;
		Y = 0.f;
		return;
	}

	const USkeletalMeshComponent* SkelComp = Context.AnimInstanceProxy->GetSkelMeshComponent();
	if (!SkelComp || !SkelComp->GetOwner() || !Character)
	{
		return;
	}

	// Cache vars for use
	const FBoneContainer& BoneContainer = Context.AnimInstanceProxy->GetRequiredBones();
	const FTransform& MeshTM = Context.AnimInstanceProxy->GetComponentTransform();
	UWorld* const World = Context.AnimInstanceProxy->GetAnimInstanceObject()->GetWorld();

	if (!World) { return; }

	FHitResult& Hit = PredictedLanding.HitResult;

	// Determine direction and lateral distance
	{
		// Get rotation to predict location
		const FRotator DeltaRot = (Hit.Location - MeshTM.GetLocation()).Rotation();

		FRotator LegRotation = MeshTM.InverseTransformRotation(DeltaRot.Quaternion()).Rotator() - FRotator(0.f, 90.f, 0.f);
		LegRotation.Normalize();

		const float LateralDistance = (MeshTM.GetLocation() - Hit.Location).Size2D();

		if (FMath::IsNearlyZero(LateralDistance, 1.f))
		{
			LegRotation.Yaw = 0.f;
		}
		X = LegRotation.Yaw;

		const float InterpRate = (LateralDistance >= Y) ? LateralInterpUpRate : LateralInterpDownRate;
		if (InterpRate != 0.f)
		{
			Y = FMath::FInterpConstantTo(Y, LateralDistance, Context.GetDeltaTime(), InterpRate);
		}
		else
		{
			Y = LateralDistance;
		}
	}
}

bool FAnimNode_LandingPose::PredictLandingLocation(FPredictProjectilePathResult& OutPredictResult)
{
	if (!Character) { return false; }

	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.98f;
	const float MaxSimTime = 2.f;
	const float SimFrequency = 15.f;
	const float GravityZ = Character->GetCharacterMovement()->GetGravityZ();
	const TArray<AActor*> TraceIgnore{ Character };

	FPredictProjectilePathParams Params = FPredictProjectilePathParams(Radius, Character->GetActorLocation(), OwnerVelocity, MaxSimTime);
	Params.bTraceWithCollision = true;
	Params.bTraceComplex = false;
	Params.ActorsToIgnore = TraceIgnore;
	Params.DrawDebugType = EDrawDebugTrace::None;
	Params.DrawDebugTime = 1.f;
	Params.SimFrequency = SimFrequency;
	Params.OverrideGravityZ = GravityZ;
	Params.TraceChannel = ECollisionChannel::ECC_Visibility; // Trace by channel

	// Do the trace
	if (!FMIStatics::PredictCapsulePath(Character, HalfHeight, Params, OutPredictResult))
	{
		return false;
	}
	return true;
}
