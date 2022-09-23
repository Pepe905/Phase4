// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blutility/Classes/EditorUtilityWidget.h"
#include "MIAnimRetargetSorter.generated.h"

/**
 * 
 */
UCLASS()
class MOVEITEDITORTOOLS_API UMIAnimRetargetSorter : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Development|Editor")
	TArray<FString> GetSelectedDirectories();

	UFUNCTION(BlueprintCallable, Category = "Development|Editor")
	void ArrangeRetargetedAnimations(const FString& Path, TArray<UObject*> SelectedObjects, const FString& Prefix, const FString& Suffix, const FString& Replace, const FString& ReplaceWith);

	void FixUpRedirectors(TArray<UObject*> SelectedObjects);

	void ExecuteFixUpRedirectorsInFolder(FString AnimFolder);
	
	UFUNCTION(BlueprintCallable, Category = "Development|Editor")
	FORCEINLINE bool ObjectInArrayHasClass(const TArray<UObject*>& Array, TSubclassOf<UObject> Class) const
	{
		for (const UObject* const Obj : Array)
		{
			if (Obj->IsA(Class))
			{
				return true;
			}
		}
		return false;
	}
};
