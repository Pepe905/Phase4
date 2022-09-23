// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#include "MoveItEditorTools.h"
#include "MIAnimInstanceCustomization.h"
#include "MICharacterCustomization.h"

#define LOCTEXT_NAMESPACE "FMoveItEditorToolsModule"

void FMoveItEditorToolsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	auto& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >("PropertyEditor");

	// Register our customization to be used by a class 'UMyClass' or 'AMyClass'. Note the prefix must be dropped.
	PropertyModule.RegisterCustomClassLayout(
		"MIAnimInstance",
		FOnGetDetailCustomizationInstance::CreateStatic(&FMIAnimInstanceCustomization::MakeInstance)
	);

	PropertyModule.RegisterCustomClassLayout(
		"MICharacter",
		FOnGetDetailCustomizationInstance::CreateStatic(&FMICharacterCustomization::MakeInstance)
	);

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FMoveItEditorToolsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMoveItEditorToolsModule, MoveItEditorTools)
