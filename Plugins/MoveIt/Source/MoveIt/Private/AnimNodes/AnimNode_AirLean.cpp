// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_AirLean.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstanceProxy.h"


void FAnimNode_AirLean::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_AdditiveBlendSpace::Initialize_AnyThread(Context);

	const USkeletalMeshComponent* const SkelMesh = Context.AnimInstanceProxy->GetSkelMeshComponent();
	Character = (SkelMesh && SkelMesh->GetOwner()) ? Cast<ACharacter>(SkelMesh->GetOwner()) : nullptr;
}

void FAnimNode_AirLean::UpdateBlendSpace(const FAnimationUpdateContext& Context)
{
	if (!Character || !Character->GetCharacterMovement() || Character->GetCharacterMovement()->MovementMode == MOVE_None)
	{
		return;
	}

	const FVector& Velocity = Character->GetVelocity();
	const float JumpZ = Character->GetCharacterMovement()->JumpZVelocity;

	// Compute lean amount
	const float V = UKismetMathLibrary::MapRangeClamped(Velocity.Z, JumpZ, JumpZ * 2.f, 1.f, -1.f);
	const float S = UKismetMathLibrary::NormalizeToRange(Velocity.Size2D(), 0.f, Character->GetCharacterMovement()->GetMaxSpeed());
	Y = V * S;

	// Compute lean direction
	X = (Character->GetActorRotation().RotateVector(Velocity).Rotation()).GetNormalized().Yaw;
}
