// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "MIPlayerController.h"

void AMIPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	K2_SetPawn(InPawn);
}
