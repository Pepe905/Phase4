// Copyright (c) 2019-2021 Drowning Dragons Limited. All Rights Reserved.

#pragma once

#include "IDetailCustomization.h"

class FMIAnimInstanceCustomization: public IDetailCustomization
{
public:
    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
    //

    static TSharedRef< IDetailCustomization > MakeInstance();
};