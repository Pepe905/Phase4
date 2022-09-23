// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "Templates/MICharacter_TwinStick.h"
#include "GameFramework/PlayerController.h"

AMICharacter_TwinStick::AMICharacter_TwinStick(const FObjectInitializer& OI)
	: Super(OI)
{
}

void AMICharacter_TwinStick::OnReceiveController(AController* NewController)
{
	PlayerController = (NewController) ? Cast<APlayerController>(NewController) : nullptr;

	K2_OnReceiveController(NewController);
	K2_OnReceivePlayerController(PlayerController);
}

void AMICharacter_TwinStick::SetCustomGameInputMode()
{
	if (PlayerController)
	{
		FInputModeGameOnly GIM;
		GIM.SetConsumeCaptureMouseDown(false);
		PlayerController->SetInputMode(GIM);
	}
}

void AMICharacter_TwinStick::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (GetController() && GetController()->IsLocalController())
	{
		// Local Client
		OnReceiveController(GetController());
	}
}

void AMICharacter_TwinStick::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (GetController())
	{
		// Server & Standalone
		OnReceiveController(NewController);
	}
}

void AMICharacter_TwinStick::ReceiveMouseTurnInput(float AxisValue)
{
	if (!PlayerController) { return; }

	// Don't respond to minor movements
	if (!FMath::IsNearlyZero(AxisValue, 0.01f))
	{
		// Get mouse location & direction in world space
		FVector WorldLocation = FVector::ZeroVector;
		FVector WorldDirection = FVector::ZeroVector;
		if (PlayerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
		{
			FRotator NewRotation = FRotator::ZeroRotator;
			if (GetMouseFacingRotation(WorldLocation, WorldDirection, NewRotation))
			{
				UpdateControlRotation(NewRotation);
			}
		}
	}
}

void AMICharacter_TwinStick::ReceiveGamepadTurnInput(FVector2D AxisValues)
{
	if (!PlayerController) { return; }

	// Don't respond to minor movements
	if (!FMath::IsNearlyZero(AxisValues.X, 0.01f) || !FMath::IsNearlyZero(AxisValues.Y, 0.01f))
	{
		const FVector InputDirection = FVector(-AxisValues.Y, AxisValues.X, 0.f);
		FRotator NewRotation = FRotator::ZeroRotator;
		if (GetGamepadFacingRotation(InputDirection, NewRotation))
		{
			UpdateControlRotation(NewRotation);
		}
	}
}

bool AMICharacter_TwinStick::GetMouseFacingRotation_Implementation(FVector WorldLocation, FVector WorldDirection, FRotator& NewRotation) const
{
	NewRotation = FRotator::ZeroRotator;

	// Make it a usable value (extend outwards)
	WorldDirection *= 50000.f;

	// Intersect mouse location and direction with our character
	float T = 0.f;
	FVector Intersection = FVector::ZeroVector;
	if (UKismetMathLibrary::LinePlaneIntersection_OriginNormal(WorldLocation, WorldLocation + WorldDirection, GetActorLocation(), FVector::UpVector, T, Intersection))
	{
		NewRotation = UKismetMathLibrary::FindLookAtRotation(FVector(GetActorLocation().X, GetActorLocation().Y, 0.f), FVector(Intersection.X, Intersection.Y, 0.f));
		return true;
	}

	return false;
}

bool AMICharacter_TwinStick::GetGamepadFacingRotation_Implementation(FVector WorldDirection, FRotator& NewRotation) const
{
	// Make it a usable value (extend outwards)
	WorldDirection *= 50000.f;

	NewRotation = UKismetMathLibrary::FindLookAtRotation(FVector(GetActorLocation().X, GetActorLocation().Y, 0.f), WorldDirection);
	return true;
}

void AMICharacter_TwinStick::UpdateControlRotation_Implementation(const FRotator& NewControlRotation)
{
	if (PlayerController)
	{
		PlayerController->SetControlRotation(NewControlRotation);
	}
}
