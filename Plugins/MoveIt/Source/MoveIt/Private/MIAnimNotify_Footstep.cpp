// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "MIAnimNotify_Footstep.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "MIPhysicalMaterial.h"
#include "MICharacter.h"
#include "Curves/CurveFloat.h"
#include "Sound/SoundBase.h"
#include "Components/SkeletalMeshComponent.h"


DEFINE_LOG_CATEGORY_STATIC(LogParticles, Log, All);

FString UMIAnimNotify_Footstep::GetNotifyName_Implementation() const
{
	return "Footstep";
}

void UMIAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (SocketName.IsNone() || !MeshComp->DoesSocketExist(SocketName))
	{
		return;
	}

	FTransform TM = MeshComp->GetSocketTransform(SocketName);

#if WITH_EDITORONLY_DATA
	if (MeshComp->GetWorld() && (!MeshComp->GetWorld()->IsGameWorld() && !bEnableOutsideGameWorld))
	{
		UGameplayStatics::PlaySoundAtLocation(MeshComp->GetWorld(), PreviewSound, TM.GetLocation(), VolumeMultiplier, PitchMultiplier);
		UGameplayStatics::SpawnEmitterAtLocation(MeshComp->GetWorld(), PreviewPSTemplate, TM, true, EPSCPoolMethod::AutoRelease);
		return;
	}
#endif

	if (MeshComp->GetWorld() && (MeshComp->GetWorld()->IsGameWorld() || bEnableOutsideGameWorld))
	{
		TArray<AActor*> TraceIgnore;
		if (MeshComp->GetOwner())
		{
			TraceIgnore.Add(MeshComp->GetOwner());
		}

		FHitResult Hit(ForceInit);

		if (UKismetSystemLibrary::LineTraceSingle(
			MeshComp,
			TM.GetLocation(),
			TM.GetLocation() - FVector::UpVector * 15.f,
			UEngineTypes::ConvertToTraceType(ECC_Visibility),
			true,
			TraceIgnore,
			EDrawDebugTrace::None,
			Hit,
			false
		))
		{
			TM.SetLocation(Hit.ImpactPoint);

			UMIPhysicalMaterial* PhysMat = Hit.PhysMaterial.IsValid() ? Cast<UMIPhysicalMaterial>(Hit.PhysMaterial.Get()) : nullptr;
			if (!PhysMat && MeshComp->GetOwner())
			{
				if (AMICharacter* const Character = Cast<AMICharacter>(MeshComp->GetOwner()))
				{
					PhysMat = Character->DefaultPhysicalMaterial;
				}
			}
			if (PhysMat)
			{
				// Play sound effect
				USoundBase* const SoundToPlay = PhysMat->BoneImpactSound;
				if (SoundToPlay)
				{
					const FRuntimeFloatCurve& VolumeCurve = PhysMat->BoneImpactVelocityToVolume;
					const FRuntimeFloatCurve& PitchCurve = PhysMat->BoneImpactVelocityToPitch;

					if (VolumeMultiplier > 0.f && PitchMultiplier > 0.f)
					{
						UGameplayStatics::PlaySoundAtLocation(MeshComp, SoundToPlay, Hit.ImpactPoint, VolumeMultiplier, PitchMultiplier);
					}
				}

				// Play particle effect
				UParticleSystem* const ParticleToPlay = PhysMat != nullptr ? PhysMat->BoneImpactParticle : nullptr;
				if (ParticleToPlay)
				{
					const FTransform ParticleTransform(TM.GetRotation(), Hit.ImpactPoint, FVector::OneVector);
					UGameplayStatics::SpawnEmitterAtLocation(MeshComp->GetWorld(), ParticleToPlay, ParticleTransform, true, EPSCPoolMethod::AutoRelease);
				}
			}
		}
	}
}

#if WITH_EDITOR
void UMIAnimNotify_Footstep::ValidateAssociatedAssets()
{
	static const FName NAME_AssetCheck("AssetCheck");

	if ((PreviewSound != nullptr) && (PreviewSound->IsLooping()))
	{
		UObject* ContainingAsset = GetContainingAsset();

		FMessageLog AssetCheckLog(NAME_AssetCheck);

		const FText MessageLooping = FText::Format(
			NSLOCTEXT("AnimNotify", "Sound_ShouldNotLoop", "Sound {0} used in anim notify for asset {1} is set to looping, but the slot is a one-shot (it won't be played to avoid leaking an instance per notify)."),
			FText::AsCultureInvariant(PreviewSound->GetPathName()),
			FText::AsCultureInvariant(ContainingAsset->GetPathName()));
		AssetCheckLog.Warning()
			->AddToken(FUObjectToken::Create(ContainingAsset))
			->AddToken(FTextToken::Create(MessageLooping));

		if (GIsEditor)
		{
			AssetCheckLog.Notify(MessageLooping, EMessageSeverity::Warning, /*bForce=*/ true);
		}
	}

	if ((PreviewPSTemplate != nullptr) && (PreviewPSTemplate->IsLooping()))
	{
		UObject* ContainingAsset = GetContainingAsset();

		FMessageLog AssetCheckLog(NAME_AssetCheck);

		const FText MessageLooping = FText::Format(
			NSLOCTEXT("AnimNotify", "ParticleSystem_ShouldNotLoop", "Particle system {0} used in anim notify for asset {1} is set to looping, but the slot is a one-shot (it won't be played to avoid leaking a component per notify)."),
			FText::AsCultureInvariant(PreviewPSTemplate->GetPathName()),
			FText::AsCultureInvariant(ContainingAsset->GetPathName()));
		AssetCheckLog.Warning()
			->AddToken(FUObjectToken::Create(ContainingAsset))
			->AddToken(FTextToken::Create(MessageLooping));

		if (GIsEditor)
		{
			AssetCheckLog.Notify(MessageLooping, EMessageSeverity::Warning, /*bForce=*/ true);
		}
	}
}
#endif
