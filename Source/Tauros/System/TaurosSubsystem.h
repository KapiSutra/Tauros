// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include <eos_common.h>

#include "CoreMinimal.h"
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

	//

	void Eos_ExternalToken_Authorize(const ULocalPlayer* LocalPlayer,
	                                 const FString& TokenType,
	                                 EOS_EExternalAccountType AccountType,
	                                 EOS_EExternalCredentialType CredentialType);


	UE_DEPRECATED(5, "Use Dwebble Oidc")
	UFUNCTION(BlueprintCallable)
	void Eos_OidcAuthCode_Authorize(const ULocalPlayer* LocalPlayer, const FString& Issuer);

protected:
	FTSTicker::FDelegateHandle TickHandle;

	// EOS
	// UE::Online::TOnlineAsyncOpHandle<UE::Online::FAuthLogin> LoginHandle;


	UE::Online::IOnlineServicesPtr OnlineServices;

	struct FOnlineServicesInfo
	{
		UE::Online::IOnlineServicesPtr OnlineServices = nullptr;

		void Reset()
		{
			OnlineServices.Reset();
		}
	};

	FOnlineServicesInfo OnlineServicesInfo;

	TOptional<uint32> Oidc_LoopbackPort = {19654};
};
