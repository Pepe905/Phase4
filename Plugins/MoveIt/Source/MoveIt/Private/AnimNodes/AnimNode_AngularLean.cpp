// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "AnimNodes/AnimNode_AngularLean.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PawnMovementComponent.h"

void FAnimNode_AngularLean::PreUpdate(const UAnimInstance* InAnimInstance)
{
	FAnimNode_AdditiveAngularBase::PreUpdate(InAnimInstance);

	const APawn* const PawnOwner = InAnimInstance->TryGetPawnOwner();
	if (PawnOwner && PawnOwner->GetMovementComponent())
	{
		Velocity = PawnOwner->GetVelocity();

		Speed = Velocity.Size2D();
		NormalizedSpeed = UKismetMathLibrary::NormalizeToRange(Speed, 0.f, PawnOwner->GetMovementComponent()->GetMaxSpeed());
	}
}

void FAnimNode_AngularLean::UpdateBlendSpace(const FAnimationUpdateContext& Context)
{
	X = AngularVelocity.Yaw * Scale;
	Y = NormalizedSpeed;
}
