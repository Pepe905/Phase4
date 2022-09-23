// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.


#include "MIPhysicalMaterial.h"
#include "MICharacter.h"

USoundBase* UMIPhysicalMaterial::GetHitSound(AMICharacter* const Character) const
{
	USoundBase* Result = nullptr;

	switch (HitSoundLookupType)
	{
	case EMIPhysMatLookupType::PMLT_CPP:
		Result = Character ? Character->GetHitSound(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = HitSound; }
		break;
	case EMIPhysMatLookupType::PMLT_BP:
		Result = Character ? Character->K2_GetHitSound(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = HitSound; }
		break;
	default:
		Result = HitSound;
		break;
	}

	return Result;
}

USoundBase* UMIPhysicalMaterial::GetScuffSound(AMICharacter* const Character) const
{
	USoundBase* Result = nullptr;

	switch (ScuffSoundLookupType)
	{
	case EMIPhysMatLookupType::PMLT_CPP:
		Result = Character ? Character->GetScuffSound(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = ScuffSound; }
		break;
	case EMIPhysMatLookupType::PMLT_BP:
		Result = Character ? Character->K2_GetScuffSound(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = ScuffSound; }
		break;
	default:
		Result = ScuffSound;
		break;
	}

	return Result;
}

USoundBase* UMIPhysicalMaterial::GetBoneImpactSound(AMICharacter* const Character) const
{
	USoundBase* Result = nullptr;

	switch (BoneImpactSoundLookupType)
	{
	case EMIPhysMatLookupType::PMLT_CPP:
		Result = Character ? Character->GetBoneImpactSound(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = BoneImpactSound; }
		break;
	case EMIPhysMatLookupType::PMLT_BP:
		Result = Character ? Character->K2_GetBoneImpactSound(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = BoneImpactSound; }
		break;
	default:
		Result = BoneImpactSound;
		break;
	}

	return Result;
}

UNiagaraSystem* UMIPhysicalMaterial::GetHitNiagara(AMICharacter* const Character) const
{
	UNiagaraSystem* Result = nullptr;

	if (ParticleSystemType == EMIParticleSystemType::MIPST_Legacy)
	{
		return nullptr;
	}

	switch (HitParticleLookupType)
	{
	case EMIPhysMatLookupType::PMLT_CPP:
		Result = Character ? Character->GetHitNiagara(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = HitNiagara; }
		break;
	case EMIPhysMatLookupType::PMLT_BP:
		Result = Character ? Character->K2_GetHitNiagara(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = HitNiagara; }
		break;
	default:
		Result = HitNiagara;
		break;
	}

	return Result;
}

UNiagaraSystem* UMIPhysicalMaterial::GetScuffNiagara(AMICharacter* const Character) const
{
	UNiagaraSystem* Result = nullptr;

	if (ParticleSystemType == EMIParticleSystemType::MIPST_Legacy)
	{
		return nullptr;
	}

	switch (ScuffParticleLookupType)
	{
	case EMIPhysMatLookupType::PMLT_CPP:
		Result = Character ? Character->GetScuffNiagara(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = ScuffNiagara; }
		break;
	case EMIPhysMatLookupType::PMLT_BP:
		Result = Character ? Character->K2_GetScuffNiagara(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = ScuffNiagara; }
		break;
	default:
		Result = ScuffNiagara;
		break;
	}

	return Result;
}

UNiagaraSystem* UMIPhysicalMaterial::GetBoneImpactNiagara(AMICharacter* const Character) const
{
	UNiagaraSystem* Result = nullptr;

	if (ParticleSystemType == EMIParticleSystemType::MIPST_Legacy)
	{
		return nullptr;
	}

	switch (BoneImpactParticleLookupType)
	{
	case EMIPhysMatLookupType::PMLT_CPP:
		Result = Character ? Character->GetBoneImpactNiagara(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = BoneImpactNiagara; }
		break;
	case EMIPhysMatLookupType::PMLT_BP:
		Result = Character ? Character->K2_GetBoneImpactNiagara(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = BoneImpactNiagara; }
		break;
	default:
		Result = BoneImpactNiagara;
		break;
	}

	return Result;
}

UParticleSystem* UMIPhysicalMaterial::GetHitParticle(AMICharacter* const Character) const
{
	UParticleSystem* Result = nullptr;

	if (ParticleSystemType == EMIParticleSystemType::MIPST_Niagara)
	{
		return nullptr;
	}

	switch (HitParticleLookupType)
	{
	case EMIPhysMatLookupType::PMLT_CPP:
		Result = Character ? Character->GetHitParticle(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = HitParticle; }
		break;
	case EMIPhysMatLookupType::PMLT_BP:
		Result = Character ? Character->K2_GetHitParticle(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = HitParticle; }
		break;
	default:
		Result = HitParticle;
		break;
	}

	return Result;
}

UParticleSystem* UMIPhysicalMaterial::GetScuffParticle(AMICharacter* const Character) const
{
	UParticleSystem* Result = nullptr;

	if (ParticleSystemType == EMIParticleSystemType::MIPST_Niagara)
	{
		return nullptr;
	}

	switch (ScuffParticleLookupType)
	{
	case EMIPhysMatLookupType::PMLT_CPP:
		Result = Character ? Character->GetScuffParticle(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = ScuffParticle; }
		break;
	case EMIPhysMatLookupType::PMLT_BP:
		Result = Character ? Character->K2_GetScuffParticle(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = ScuffParticle; }
		break;
	default:
		Result = ScuffParticle;
		break;
	}

	return Result;
}

UParticleSystem* UMIPhysicalMaterial::GetBoneImpactParticle(AMICharacter* const Character) const
{
	UParticleSystem* Result = nullptr;

	if (ParticleSystemType == EMIParticleSystemType::MIPST_Niagara)
	{
		return nullptr;
	}

	switch (BoneImpactParticleLookupType)
	{
	case EMIPhysMatLookupType::PMLT_CPP:
		Result = Character ? Character->GetBoneImpactParticle(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = BoneImpactParticle; }
		break;
	case EMIPhysMatLookupType::PMLT_BP:
		Result = Character ? Character->K2_GetBoneImpactParticle(this) : nullptr;
		if (!Result && FallbackMode == EMIPhysMatLookupFallback::PMLF_Material) { Result = BoneImpactParticle; }
		break;
	default:
		Result = BoneImpactParticle;
		break;
	}

	return Result;
}
