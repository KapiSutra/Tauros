// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TaurosOidcTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UE5Coro.h"
#include "TaurosOidcSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class TAUROS_API UTaurosOidcSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "Tauros | Oidc")
	FVoidCoroutine BrowserOidc(FTaurosOidcParams Params, FTaurosOidcResult& Result);
};
