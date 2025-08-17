// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TaurosOidcTypes.generated.h"

USTRUCT(BlueprintType)
struct TAUROS_API FTaurosOidcParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Issuer;

	UPROPERTY(BlueprintReadWrite)
	FString ClientId;

	// UPROPERTY(BlueprintReadWrite)
	TOptional<FString> ClientSecret;

	UPROPERTY(BlueprintReadWrite)
	int32 LoopbackPort;

	UPROPERTY(BlueprintReadWrite)
	int32 LoopbackRoute;
};


USTRUCT(BlueprintType)
struct TAUROS_API FTaurosOidcResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	bool bSuccess;

	UPROPERTY(BlueprintReadWrite)
	FString AccessToken;
};
