// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "MITypes.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Components/SkeletalMeshComponent.h"

void FPhysicsBlend::Impact(USkeletalMeshComponent* const Mesh, const FVector& ImpactNormal, const float ImpactMagnitude)
{
	if (CanSimulate(Mesh))
	{
		if (!IsActive())
		{
			BlendIn.Reset();
			BlendOut.Reset();

			Mesh->SetAllBodiesBelowSimulatePhysics(BoneName, true);
			Mesh->SetAllBodiesBelowPhysicsBlendWeight(BoneName, MinBlendWeight);

			PhysicsBlendState = EPhysicsBlendState::PBS_In;
		}

		const FVector Impulse = ImpactNormal * ImpactMagnitude * ImpulseMultiplier;
		Mesh->AddAngularImpulseInDegrees(Impulse.GetClampedToMaxSize(MaxImpulseTaken), BoneName, true);
	}
}

bool FPhysicsBlend::Update(USkeletalMeshComponent* const Mesh, float DeltaTime)
{
	if (IsActive() && Mesh)
	{
		FAlphaBlend& Blend = GetBlend();
		Blend.Update(DeltaTime);

		if (Blend.IsComplete())
		{
			if (PhysicsBlendState == EPhysicsBlendState::PBS_In)
			{
				PhysicsBlendState = EPhysicsBlendState::PBS_Out;
			}
			else
			{
				PhysicsBlendState = EPhysicsBlendState::PBS_Inactive;

				Mesh->SetAllBodiesBelowPhysicsBlendWeight(BoneName, 0.f);

				// Return true on completion
				return true;
			}
		}

		// Update because it changed
		Blend = GetBlend();

		const float Alpha = (PhysicsBlendState == EPhysicsBlendState::PBS_In) ? Blend.GetAlpha() : 1.f - Blend.GetAlpha();
		const float BlendWeight = UKismetMathLibrary::MapRangeClamped(Alpha, 0.f, 1.f, MinBlendWeight, MaxBlendWeight);

		Mesh->SetAllBodiesBelowPhysicsBlendWeight(BoneName, BlendWeight);
	}

	// Is completed
	return !IsActive();
}

bool FPhysicsBlend::CanSimulate(USkeletalMeshComponent* const Mesh) const
{
	if (!Mesh || !Mesh->GetOwner())
	{
		return false;
	}

	if (Mesh->GetOwnerRole() != ENetRole::ROLE_AutonomousProxy && Mesh->GetOwnerRole() != ENetRole::ROLE_SimulatedProxy)
	{
		if (Mesh->GetNetMode() != NM_Standalone)
		{
			return false;
		}
	}

	if (!Mesh->DoesSocketExist(BoneName))
	{
		return false;
	}

	if (Mesh->GetCollisionEnabled() < ECollisionEnabled::PhysicsOnly)
	{
		UE_LOG(LogTemp, Warning, TEXT("{ %s } Invalid Collision Enabled - Must enable physics! This is used with the footstep impact system"), *Mesh->GetName());
		return false;
	}

	return true;
}

bool FMIStatics::PredictCapsulePath(const UObject* WorldContextObject, float HalfHeight, const struct FPredictProjectilePathParams& PredictParams, struct FPredictProjectilePathResult& PredictResult)
{
	PredictResult.Reset();
	bool bBlockingHit = false;

	UWorld const* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World && PredictParams.SimFrequency > KINDA_SMALL_NUMBER)
	{
		const float SubstepDeltaTime = 1.f / PredictParams.SimFrequency;
		const float GravityZ = FMath::IsNearlyEqual(PredictParams.OverrideGravityZ, 0.0f) ? World->GetGravityZ() : PredictParams.OverrideGravityZ;
		const float ProjectileRadius = PredictParams.ProjectileRadius;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PredictProjectilePath), PredictParams.bTraceComplex);
		FCollisionObjectQueryParams ObjQueryParams;
		const bool bTraceWithObjectType = (PredictParams.ObjectTypes.Num() > 0);
		const bool bTracePath = PredictParams.bTraceWithCollision && (PredictParams.bTraceWithChannel || bTraceWithObjectType);
		if (bTracePath)
		{
			QueryParams.AddIgnoredActors(PredictParams.ActorsToIgnore);
			if (bTraceWithObjectType)
			{
				for (auto Iter = PredictParams.ObjectTypes.CreateConstIterator(); Iter; ++Iter)
				{
					const ECollisionChannel& Channel = UCollisionProfile::Get()->ConvertToCollisionChannel(false, *Iter);
					ObjQueryParams.AddObjectTypesToQuery(Channel);
				}
			}
		}

		FVector CurrentVel = PredictParams.LaunchVelocity;
		FVector TraceStart = PredictParams.StartLocation;
		FVector TraceEnd = TraceStart;
		float CurrentTime = 0.f;
		PredictResult.PathData.Reserve(FMath::Min(128, FMath::CeilToInt(PredictParams.MaxSimTime * PredictParams.SimFrequency)));
		PredictResult.AddPoint(TraceStart, CurrentVel, CurrentTime);

		FHitResult ObjectTraceHit(NoInit);
		FHitResult ChannelTraceHit(NoInit);
		ObjectTraceHit.Time = 1.f;
		ChannelTraceHit.Time = 1.f;

		const float MaxSimTime = PredictParams.MaxSimTime;
		while (CurrentTime < MaxSimTime)
		{
			// Limit step to not go further than total time.
			const float PreviousTime = CurrentTime;
			const float ActualStepDeltaTime = FMath::Min(MaxSimTime - CurrentTime, SubstepDeltaTime);
			CurrentTime += ActualStepDeltaTime;

			// Integrate (Velocity Verlet method)
			TraceStart = TraceEnd;
			FVector OldVelocity = CurrentVel;
			CurrentVel = OldVelocity + FVector(0.f, 0.f, GravityZ * ActualStepDeltaTime);
			TraceEnd = TraceStart + (OldVelocity + CurrentVel) * (0.5f * ActualStepDeltaTime);
			PredictResult.LastTraceDestination.Set(TraceEnd, CurrentVel, CurrentTime);

			if (bTracePath)
			{
				bool bObjectHit = false;
				bool bChannelHit = false;
				if (bTraceWithObjectType)
				{
					bObjectHit = World->SweepSingleByObjectType(ObjectTraceHit, TraceStart, TraceEnd, FQuat::Identity, ObjQueryParams, FCollisionShape::MakeCapsule(ProjectileRadius, HalfHeight), QueryParams);
				}
				if (PredictParams.bTraceWithChannel)
				{
					bChannelHit = World->SweepSingleByChannel(ChannelTraceHit, TraceStart, TraceEnd, FQuat::Identity, PredictParams.TraceChannel, FCollisionShape::MakeCapsule(ProjectileRadius, HalfHeight), QueryParams);
				}

				// See if there were any hits.
				if (bObjectHit || bChannelHit)
				{
					// Hit! We are done. Choose trace with earliest hit time.
					PredictResult.HitResult = (ObjectTraceHit.Time < ChannelTraceHit.Time) ? ObjectTraceHit : ChannelTraceHit;
					const float HitTimeDelta = ActualStepDeltaTime * PredictResult.HitResult.Time;
					const float TotalTimeAtHit = PreviousTime + HitTimeDelta;
					const FVector VelocityAtHit = OldVelocity + FVector(0.f, 0.f, GravityZ * HitTimeDelta);
					PredictResult.AddPoint(PredictResult.HitResult.Location, VelocityAtHit, TotalTimeAtHit);
					bBlockingHit = true;
					break;
				}
			}

			PredictResult.AddPoint(TraceEnd, CurrentVel, CurrentTime);
		}

		// Draw debug path
#if ENABLE_DRAW_DEBUG
		if (PredictParams.DrawDebugType != EDrawDebugTrace::None)
		{
			const bool bPersistent = PredictParams.DrawDebugType == EDrawDebugTrace::Persistent;
			const float LifeTime = (PredictParams.DrawDebugType == EDrawDebugTrace::ForDuration) ? PredictParams.DrawDebugTime : 0.f;
			const float DrawRadius = (ProjectileRadius > 0.f) ? ProjectileRadius : 5.f;

			// draw the path
			for (const FPredictProjectilePathPointData& PathPt : PredictResult.PathData)
			{
				::DrawDebugCapsule(World, PathPt.Location, HalfHeight, DrawRadius, FQuat::Identity, FColor::Green, bPersistent, LifeTime);
			}
			// draw the impact point
			if (bBlockingHit)
			{
				::DrawDebugCapsule(World, PredictResult.HitResult.Location, HalfHeight, DrawRadius + 1.0f, FQuat::Identity, FColor::Red, bPersistent, LifeTime);
			}
		}
#endif //ENABLE_DRAW_DEBUG
	}

	return bBlockingHit;
}
