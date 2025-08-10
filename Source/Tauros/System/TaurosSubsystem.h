// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TaurosSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class TAUROS_API UTaurosSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

protected:
    FTSTicker::FDelegateHandle TickHandle;
};
