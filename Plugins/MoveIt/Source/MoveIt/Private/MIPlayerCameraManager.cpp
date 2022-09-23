// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "MIPlayerCameraManager.h"
#include "MICharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "MIAnimInstance.h"
#include "GameFramework/PlayerController.h"


void AMIPlayerCameraManager::BeginPlay()
{
	Super::BeginPlay();

	InitDefaults();
}

void AMIPlayerCameraManager::InitializeFor(class APlayerController* PC)
{
	Super::InitializeFor(PC);

	InitDefaults();
}

void AMIPlayerCameraManager::InitDefaults()
{
	// Cache default limits so they can be restored later
	if (!bInitialized)
	{
		DefaultViewYawMin = ViewYawMin;
		DefaultViewYawMax = ViewYawMax;
		bInitialized = true;
	}
}

void AMIPlayerCameraManager::OnCharacterUpdated()
{
	// IsValid checks are used here instead of nullptr checks in case a pawn is destroyed during this function

	ClearAnimInstance();

	if (GetOwningPlayerController())
	{
		// PC has no pawn possessed
		if (!GetOwningPlayerController()->GetPawn())
		{
			MICharacter = nullptr;
			return;
		}

		// Update character if PC has changed possession
		if (!IsValid(MICharacter) && GetOwningPlayerController()->GetPawn() != MICharacter)
		{
			MICharacter = (GetOwningPlayerController()->GetPawn()) ? Cast<AMICharacter>(GetOwningPlayerController()->GetPawn()) : nullptr;
		}

		// Bind events and cache anim instance
		if (IsValid(MICharacter))
		{
			CurrentAnimInstance = (MICharacter->GetMesh()->GetAnimInstance()) ? Cast<UMIAnimInstance>(MICharacter->GetMesh()->GetAnimInstance()) : nullptr;
			if (CurrentAnimInstance)
			{
				CurrentAnimInstance->OnMontageStarted.AddDynamic(this, &AMIPlayerCameraManager::OnStartMontage);
				CurrentAnimInstance->OnMontageBlendingOut.AddDynamic(this, &AMIPlayerCameraManager::OnStopMontage);

				// Montage already in progress, this isn't a good place to bind (or even possess the character) and may cause camera yaw to snap
				if (MICharacter->HasAnyRootMotion())
				{
					OnStartMontage(MICharacter->GetCurrentMontage());
				}
			}
		}
	}
	else
	{
		// No player controller
		MICharacter = nullptr;
	}
}

void AMIPlayerCameraManager::ClearAnimInstance()
{
	if (CurrentAnimInstance)
	{
		CurrentAnimInstance->OnMontageStarted.RemoveDynamic(this, &AMIPlayerCameraManager::OnStartMontage);
		CurrentAnimInstance->OnMontageBlendingOut.RemoveDynamic(this, &AMIPlayerCameraManager::OnStopMontage);
	}

	CurrentAnimInstance = nullptr;

	ViewYawMin = DefaultViewYawMin;
	ViewYawMax = DefaultViewYawMax;
}

void AMIPlayerCameraManager::OnStartMontage(UAnimMontage* Montage)
{
	if (bClampYawDuringRootMotion && Montage && Montage->HasRootMotion())
	{
		// Sanity check, not implemented by default
		if (HasValidData())
		{
			const FRotator RotationClamp = FRotator(0.f, CurrentAnimInstance->MaxTurnAngle, 0.f);
			const FRotator ControlRotation = GetOwningPlayerController()->GetControlRotation();
			if (CurrentAnimInstance->MaxTurnAngle != 0.f)
			{
				ViewYawMin = (ControlRotation - RotationClamp).GetNormalized().Yaw;
				ViewYawMax = (ControlRotation + RotationClamp).GetNormalized().Yaw;
			}
			else
			{
				// In case anim settings changed at a recent point in time
				ViewYawMin = DefaultViewYawMin;
				ViewYawMax = DefaultViewYawMax;
			}
		}
		else
		{
			OnFailSanityCheck();
		}
	}
	else
	{
		ViewYawMin = DefaultViewYawMin;
		ViewYawMax = DefaultViewYawMax;
	}
}

void AMIPlayerCameraManager::OnStopMontage(UAnimMontage* Montage, bool bInterrupted)
{
	ViewYawMin = DefaultViewYawMin;
	ViewYawMax = DefaultViewYawMax;
}

bool AMIPlayerCameraManager::HasValidData() const
{
	// No character
	if (!MICharacter)
	{
		return false;
	}

	// No anim instance
	if (!CurrentAnimInstance)
	{
		return false;
	}

	// No player controller
	if (!GetOwningPlayerController())
	{
		return false;
	}

	// Wrong pawn assigned, this needs to be taken care of
	// Eg. Override APlayerController::SetPawn()
	if (GetOwningPlayerController()->GetPawn() != MICharacter)
	{
		// Update the pawn appropriately if you hit this
		ensure(false);
		return false;
	}

	return true;
}

void AMIPlayerCameraManager::OnFailSanityCheck_Implementation()
{
	// Not used, but you can use it!

	// Be careful not to use this to call OnCharacterUpdated() in an effort to update values as
	// it could cause infinite recursion - do it properly
}
