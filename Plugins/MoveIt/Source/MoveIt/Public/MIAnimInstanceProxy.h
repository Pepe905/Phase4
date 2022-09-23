// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstanceProxy.h"
#include "MIAnimInstanceProxy.generated.h"

/**
 * Used to pass data in/out of threaded animation nodes
 * This is used specifically to get the correct position for the offhand placed on a weapon
 */
USTRUCT(meta = (DisplayName = "Native Variables"))
struct FMIAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FTransform OffHandIKTM;

	FMIAnimInstanceProxy()
		: Super()
		, OffHandIKTM(FTransform::Identity)
	{}

	FMIAnimInstanceProxy(UAnimInstance* Instance)
		: Super(Instance)
		, OffHandIKTM(FTransform::Identity)
	{}

	virtual ~FMIAnimInstanceProxy() {}
};
