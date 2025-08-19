// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include <eos_common.h>

#include "CoreMinimal.h"
#include "eos_connect_types.h"
#include "UE5Coro.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Online/Auth.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineServices.h"
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

	void InitializeOnlineServices();

	UE5Coro::TCoroutine<TOptional<FExternalAuthToken>> Eos_ExternalToken_Oss(
		const ULocalPlayer* LocalPlayer,
		const FString& TokenType,
		const EOS_EExternalAccountType AccountType,
		// const EOS_EExternalCredentialType CredentialType,
		UE5Coro::TLatentContext<> _
	);

	UE5Coro::TCoroutine<TOptional<FExternalAuthToken>> Eos_ExternalToken_Oidc(
		const FString& Issuer,
		const FString& ClientId,
		const TOptional<FString>& ClientSecret,
		const uint32 LoopbackPort,
		const FString& LoopbackRoute,
		UE5Coro::TLatentContext<> _
	);

	UE5Coro::TCoroutine< EOS_Connect_LoginCallbackInfo> Eos_ExternalAuth_Login(
		const ULocalPlayer* LocalPlayer,
		const EOS_EExternalCredentialType ExternalCredentialType,
		const FString Token
	);

#if 0
	UE_DEPRECATED (5, "Use Dwebble Oidc")
	UFUNCTION (BlueprintCallable)
	void Eos_OidcAuthCode_Authorize_Old(const ULocalPlayer* LocalPlayer, const FString& Issuer);
#endif

protected:
	FTSTicker::FDelegateHandle TickHandle;

	struct FOnlineServicesInfo
	{
		UE::Online::IOnlineServicesPtr OnlineServices = nullptr;

		void Reset()
		{
			OnlineServices.Reset();
		}
	};

	FOnlineServicesInfo OnlineServicesInfo;

	// TOptional<uint32> Oidc_LoopbackPort = {19654};
};


namespace Tauros
{
}
