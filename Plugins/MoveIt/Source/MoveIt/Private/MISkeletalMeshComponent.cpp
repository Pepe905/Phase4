// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "MISkeletalMeshComponent.h"
#include "MIAnimInstance.h"
#include "MICharacter.h"

UMISkeletalMeshComponent::UMISkeletalMeshComponent(const FObjectInitializer& OI)
	: Super(OI)
{
	RootYawOffset = 0.f;
}

void UMISkeletalMeshComponent::InitAnim(bool bForceReinit)
{
	UAnimInstance* const PreInitAnimInstance = AnimScriptInstance;

	Super::InitAnim(bForceReinit);

	if (AnimScriptInstance != PreInitAnimInstance || (!MIAnimScriptInstance && AnimScriptInstance))
	{
		// Anim Instance Changed
		MIAnimScriptInstance = (AnimScriptInstance) ? Cast<UMIAnimInstance>(AnimScriptInstance) : nullptr;
		if (GetWorld() && RootYawOffset != 0.f)
		{
			if (GetWorld()->IsGameWorld() || bEnableTurnInPlaceOutsidePIE)
			{
				AddLocalRotation(GetRootOffsetInverse());
				SetRootYawOffset(0.f);

				if (MIAnimScriptInstance)
				{
					SetRootYawOffset(MIAnimScriptInstance->RootYawOffset);
					AddLocalRotation(GetRootOffset());
				}
			}
		}

		// Notify character if anim instance changes
		if (GetOwner())
		{
			if (AMICharacter* const Character = Cast<AMICharacter>(GetOwner()))
			{
				Character->OnAnimInstanceChanged(PreInitAnimInstance);
			}
		}
	}
}

void UMISkeletalMeshComponent::InitRootYawFromReplication(float ReplicatedRootYawOffset)
{
	if (!MIAnimScriptInstance)
	{
		return;
	}

	if (GetOwner() && GetNetMode() == NM_Client)
	{
		// Remove RootYawOffset from previous frame
		AddLocalRotation(GetRootOffsetInverse());

		SetRootYawOffset(ReplicatedRootYawOffset);
		MIAnimScriptInstance->RootYawOffset = RootYawOffset;

		AddLocalRotation(GetRootOffset());
	}
}

void UMISkeletalMeshComponent::TickPose(float DeltaTime, bool bNeedsValidRootMotion)
{
	Super::TickPose(DeltaTime, bNeedsValidRootMotion);

#if WITH_EDITOR
	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		if (!bEnableTurnInPlaceOutsidePIE)
		{
			// Only update actual gameplay with turn in place
			return;
		}
	}
#endif

	if (ShouldTickAnimation() && MIAnimScriptInstance)
	{
		// Remove RootYawOffset from previous frame
		AddLocalRotation(GetRootOffsetInverse());

		// Apply RootYawOffset from current frame
		SetRootYawOffset(MIAnimScriptInstance->RootYawOffset);
		AddLocalRotation(GetRootOffset());
	}
}

void UMISkeletalMeshComponent::SetRootYawOffset(float NewRootYawOffset)
{
	RootYawOffset = NewRootYawOffset;

	// Update initial on server for replication to late joiners
	if (GetOwner() && GetOwner()->HasAuthority() && GetNetMode() != NM_Standalone)
	{
		if (!ServerCharacterOwner)
		{
			ServerCharacterOwner = Cast<AMICharacter>(GetOwner());
		}

		if (ServerCharacterOwner)
		{
			ServerCharacterOwner->InitialRootYawOffset = RootYawOffset;
		}
	}
}
