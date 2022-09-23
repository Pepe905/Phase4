// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Curves/CurveFloat.h"
#include "Niagara/Classes/NiagaraSystem.h"
#include "MIPhysicalMaterial.generated.h"

class AMICharacter;

UENUM(BlueprintType)
enum class EMIParticleSystemType : uint8
{
	MIPST_Legacy			UMETA(DisplayName = "Legacy"),
	MIPST_Niagara			UMETA(DisplayName = "Niagara"),
};

UENUM(BlueprintType)
enum class EMIPhysMatLookupType : uint8
{
	PMLT_Material			UMETA(DisplayName = "This Material", ToolTip = "[Most Performant] Take the material directly from this physical material"),
	PMLT_CPP				UMETA(DisplayName = "Code (MICharacter)", ToolTip = "Per-character ability to determine what asset to apply from code only (override from MICharacter)"),
	PMLT_BP					UMETA(DisplayName = "Blueprint (MICharacter)", ToolTip = "[High Performance Cost] Per-character ability to determine what asset to apply from Blueprint (override from MICharacter)"),
	PMLT_Max = 255			UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMIPhysMatLookupFallback : uint8
{
	PMLF_Material			UMETA(DisplayName = "This Material", ToolTip = "Fallback to assets assigned in this material if null asset is returned"),
	PMLF_None				UMETA(DisplayName = "None", ToolTip = "If null asset is returned do nothing"),
	PMLF_MAX = 255			UMETA(Hidden)
};

/**
 * Custom physical material with surface interaction effects
 */
UCLASS(BlueprintType, Blueprintable)
class MOVEIT_API UMIPhysicalMaterial : public UPhysicalMaterial
{
	GENERATED_BODY()
	
public:
	/** How to determine which sound to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lookup)
	EMIPhysMatLookupType HitSoundLookupType;

	/** How to determine which sound to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lookup)
	EMIPhysMatLookupType ScuffSoundLookupType;

	/** How to determine which sound to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lookup)
	EMIPhysMatLookupType BoneImpactSoundLookupType;

	/** How to determine which particle to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lookup)
	EMIPhysMatLookupType HitParticleLookupType;

	/** How to determine which particle to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lookup)
	EMIPhysMatLookupType ScuffParticleLookupType;

	/** How to determine which particle to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lookup)
	EMIPhysMatLookupType BoneImpactParticleLookupType;

	/**
	 * How systems handle LookupType returning a null asset from a code or blueprint function
	 * Setting FallbackMode to "This Material" means if a code or blueprint function returns a null asset, then the asset assigned to this material will be used instead
	 * Setting FallbackMode to "None" means that nothing will be used (maintains null asset)
	 * This does nothing when LookupType is "This Material"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lookup)
	EMIPhysMatLookupFallback FallbackMode;

	/** Sound played when the character runs into this surface at speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	USoundBase* HitSound;

	/** Sound played when the character walks beside/into this surface (scuffling sound) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	USoundBase* ScuffSound;

	/** Sound played when the character walks on this surface (footstep sound, usually) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (DisplayName = "Bone Impact Sound (Footstep)"))
	USoundBase* BoneImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Particle)
	EMIParticleSystemType ParticleSystemType;

	/** Particle spawned when the character runs into this surface at speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Particle, meta = (DisplayName = "Hit Particle", EditCondition = "ParticleSystemType == EMIParticleSystemType::MIPST_Niagara"))
	UNiagaraSystem* HitNiagara;

	/** Particle spawned when the character walks beside/into this surface (scuffling sound) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Particle, meta = (DisplayName = "Scuff Particle", EditCondition = "ParticleSystemType == EMIParticleSystemType::MIPST_Niagara"))
	UNiagaraSystem* ScuffNiagara;

	/** Particle spawned when the character walks on this surface (footstep sound, usually) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Particle, meta = (DisplayName = "Bone Impact Particle (Footstep)", EditCondition = "ParticleSystemType == EMIParticleSystemType::MIPST_Niagara"))
	UNiagaraSystem* BoneImpactNiagara;

	/** Particle spawned when the character runs into this surface at speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Particle, meta = (EditCondition = "ParticleSystemType == EMIParticleSystemType::MIPST_Legacy"))
	UParticleSystem* HitParticle;

	/** Particle spawned when the character walks beside/into this surface (scuffling sound) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Particle, meta = (EditCondition = "ParticleSystemType == EMIParticleSystemType::MIPST_Legacy"))
	UParticleSystem* ScuffParticle;
	
	/** Particle spawned when the character walks on this surface (footstep sound, usually) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Particle, meta = (DisplayName = "Bone Impact Particle (Footstep)", EditCondition = "ParticleSystemType == EMIParticleSystemType::MIPST_Legacy"))
	UParticleSystem* BoneImpactParticle;

	/** Maps Velocity [Time] to Volume [Value] when the character runs into this surface at speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (DisplayName = "Hit [Velocity -> Volume]"))
	FRuntimeFloatCurve HitVelocityToVolume;

	/** Maps Velocity [Time] to Pitch [Value] when the character runs into this surface at speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (DisplayName = "Hit [Velocity -> Pitch]"))
	FRuntimeFloatCurve HitVelocityToPitch;
	
	/** Maps Velocity [Time] to Volume [Value] when the character walks beside/into this surface (scuffling sound) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (DisplayName = "Scuff [Velocity -> Volume]"))
	FRuntimeFloatCurve ScuffVelocityToVolume;

	/** Maps Velocity [Time] to Pitch [Value] when the character walks beside/into this surface (scuffling sound) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (DisplayName = "Scuff [Velocity -> Pitch]"))
	FRuntimeFloatCurve ScuffVelocityToPitch;

	/**  Maps Velocity [Time] to Volume [Value] when the character walks on this surface (footstep sound) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (DisplayName = "Bone Impact (Footstep) [Velocity -> Volume]"))
	FRuntimeFloatCurve BoneImpactVelocityToVolume;

	/**  Maps Velocity [Time] to Pitch [Value] when the character walks on this surface (footstep sound) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (DisplayName = "Bone Impact (Footstep) [Velocity -> Pitch]"))
	FRuntimeFloatCurve BoneImpactVelocityToPitch;

	UMIPhysicalMaterial()
		: HitSoundLookupType(EMIPhysMatLookupType::PMLT_Material)
		, ScuffSoundLookupType(EMIPhysMatLookupType::PMLT_Material)
		, BoneImpactSoundLookupType(EMIPhysMatLookupType::PMLT_Material)
		, HitParticleLookupType(EMIPhysMatLookupType::PMLT_Material)
		, ScuffParticleLookupType(EMIPhysMatLookupType::PMLT_Material)
		, BoneImpactParticleLookupType(EMIPhysMatLookupType::PMLT_Material)
		, FallbackMode(EMIPhysMatLookupFallback::PMLF_Material)
		, HitSound(nullptr)
		, ScuffSound(nullptr)
		, BoneImpactSound(nullptr)
		, ParticleSystemType(EMIParticleSystemType::MIPST_Niagara)
		, HitParticle(nullptr)
		, ScuffParticle(nullptr)
		, BoneImpactParticle(nullptr)
	{
		HitVelocityToVolume.GetRichCurve()->AddKey(300.f, 1.0f);
		HitVelocityToVolume.GetRichCurve()->AddKey(700.f, 1.5f);
		HitVelocityToPitch.GetRichCurve()->AddKey(0.f, 1.0f);

		ScuffVelocityToVolume.GetRichCurve()->AddKey(0.f, 1.0f);
		ScuffVelocityToPitch.GetRichCurve()->AddKey(0.f, 1.0f);

		BoneImpactVelocityToVolume.GetRichCurve()->AddKey(0.f, 1.0f);
		BoneImpactVelocityToPitch.GetRichCurve()->AddKey(0.f, 1.0f);
	}

	USoundBase* GetHitSound(AMICharacter* const Character) const;
	USoundBase* GetScuffSound(AMICharacter* const Character) const;
	USoundBase* GetBoneImpactSound(AMICharacter* const Character) const;

	UNiagaraSystem* GetHitNiagara(AMICharacter* const Character) const;
	UNiagaraSystem* GetScuffNiagara(AMICharacter* const Character) const;
	UNiagaraSystem* GetBoneImpactNiagara(AMICharacter* const Character) const;

	UParticleSystem* GetHitParticle(AMICharacter* const Character) const;
	UParticleSystem* GetScuffParticle(AMICharacter* const Character) const;
	UParticleSystem* GetBoneImpactParticle(AMICharacter* const Character) const;
};
