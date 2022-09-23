// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "MICharacterCustomization.h"
#include "MICharacter.h" // The class we're customizing
#include "MICharacterMovementComponent.h"
#include "PropertyEditing.h"
#include "Kismet2/KismetEditorUtilities.h"

#define LOCTEXT_NAMESPACE "MoveItCharacterModule"

TSharedRef< IDetailCustomization > FMICharacterCustomization::MakeInstance()
{
    return MakeShareable(new FMICharacterCustomization);
}

void FMICharacterCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Forbid editing multiple objects at once (not needed / too much work for too little gain)
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	if (Objects.Num() != 1)
	{
		// If editing multiple objects simply use the default layout
		return;
	}

	// Character Actor
	TWeakObjectPtr<AMICharacter> Character = Cast<AMICharacter>(Objects[0].Get());
	if (!Character.IsValid())
	{
		return;
	}

	// Can only change defaults
	if (!Character->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject))
	{
		return;
	}

	// Button lambda functions

	auto OnOrientToView = [Character]
	{
		Character->MovementSystem = EMIMovementSystem::MS_OrientToView;
		Character->bUseControllerRotationYaw = true;
		Character->GetMICharacterMovement()->bOrientRotationToMovement = false;

		if (Character->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject))
		{
			FKismetEditorUtilities::CompileBlueprint((UBlueprint*)Character->GetClass()->ClassGeneratedBy);
			Character->MarkPackageDirty();
		}
		return FReply::Handled();
	};

	auto OnOrientMovement = [Character]
	{
		Character->MovementSystem = EMIMovementSystem::MS_OrientToMovement;
		Character->bUseControllerRotationYaw = false;
		Character->GetMICharacterMovement()->bOrientRotationToMovement = true;

		if (Character->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject))
		{
			FKismetEditorUtilities::CompileBlueprint((UBlueprint*)Character->GetClass()->ClassGeneratedBy);
			Character->MarkPackageDirty();
		}
		return FReply::Handled();
	};

	auto OnOrientMovementCycle = [Character]
	{
		Character->MovementSystem = EMIMovementSystem::MS_CycleOrientToMovement;
		Character->bUseControllerRotationYaw = false;
		Character->GetMICharacterMovement()->bOrientRotationToMovement = true;

		if (Character->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject))
		{
			FKismetEditorUtilities::CompileBlueprint((UBlueprint*)Character->GetClass()->ClassGeneratedBy);
			Character->MarkPackageDirty();
		}
		return FReply::Handled();
	};

	// Make category
	IDetailCategoryBuilder& Cat = DetailBuilder.EditCategory(TEXT("Apply Default Movement System"), FText::GetEmpty(), ECategoryPriority::Important);

    // Add buttons to set defaults for custom movement modes
	Cat.AddCustomRow(LOCTEXT("MovSysPreset", "Apply Default Movement System"))
		.NameContent()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(SButton).Text(LOCTEXT("onOrient", "Orient To View")).OnClicked_Lambda(OnOrientToView)
				]
			]
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(SButton).Text(LOCTEXT("onTP", "Orient To Movement")).OnClicked_Lambda(OnOrientMovement)
				]
			]
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(SButton).Text(LOCTEXT("onTPC", "Orient To Movement with Cycle")).OnClicked_Lambda(OnOrientMovementCycle)
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE