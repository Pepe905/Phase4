// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "MIAnimInstanceCustomization.h"
#include "MIAnimInstance.h" // The class we're customizing
#include "PropertyEditing.h"

#define LOCTEXT_NAMESPACE "MoveItEditorToolsModule"

TSharedRef< IDetailCustomization > FMIAnimInstanceCustomization::MakeInstance()
{
    return MakeShareable(new FMIAnimInstanceCustomization);
}

void FMIAnimInstanceCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // All this just to make the animation settings be the first thing in the details panel!

    IDetailCategoryBuilder& Cat = DetailBuilder.EditCategory("AnimationSettings", FText::GetEmpty(), ECategoryPriority::Important);
}

#undef LOCTEXT_NAMESPACE