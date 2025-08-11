// Copyright 2019-Present tarnishablec. All Rights Reserved.


#include "TaurosSubsystem.h"

#include <eos_common.h>

#include "EOSShared.h"

#include "OnlineSubsystemUtils.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineServicesEngineUtils.h"
// #include "steam/isteamuser.h"


void UTaurosSubsystem::Deinitialize()
{
    Super::Deinitialize();
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    OnlineServicesInfo.Reset();
}

void UTaurosSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const auto GameInstance = GetGameInstance();

    const auto IsServer = GameInstance->IsDedicatedServerInstance();

    if (!IsServer)
    {
        InitializeOnlineServices();

        TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float DeltaTime)
        {
            return true;
        }));


        GameInstance->OnLocalPlayerAddedEvent.AddLambda([this](ULocalPlayer* LocalPlayer)
        {
            if (GetGameInstance()->GetFirstGamePlayer() == LocalPlayer)
            {
                EosExternalLogin(
                    LocalPlayer,
                    EOS_EExternalAccountType::EOS_EAT_STEAM,
                    EOS_EExternalCredentialType::EOS_ECT_STEAM_SESSION_TICKET
                );
            }
        });
    }
}

bool UTaurosSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return Super::ShouldCreateSubsystem(Outer);
}

void UTaurosSubsystem::InitializeOnlineServices()
{
    OnlineServicesInfo.OnlineServices = UE::Online::GetServices();
    if (!OnlineServicesInfo.OnlineServices.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Error: Failed to initialize services."));
        return;
    }
}

void UTaurosSubsystem::EosExternalLogin(const ULocalPlayer* LocalPlayer, const EOS_EExternalAccountType AccountType,
                                        EOS_EExternalCredentialType CredentialType)
{
    const auto Oss = Online::GetSubsystem(GetWorld(), LexToString(AccountType));

    if (!Oss) { return; }

    const auto Identity = Oss->GetIdentityInterface();

    if (!Identity) { return; }

    const auto Delegate = IOnlineIdentity::FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateLambda(
        [this,CredentialType]
    (const int32 LocalUserNum, const bool bWasSuccessful, const FExternalAuthToken& AuthToken)
        {
            if (!bWasSuccessful)
            {
                return;
            }

            const auto PlatformUserId = GetGameInstance()->GetLocalPlayerByIndex(LocalUserNum)->
                                                           GetPlatformUserId();

            UE::Online::FAuthLogin::Params Params{
                .PlatformUserId = PlatformUserId,
                .CredentialsType = UE::Online::LoginCredentialsType::ExternalAuth,
            };


            UE::Online::FExternalAuthToken CredentialsToken{
                .Type = LexToString(CredentialType),
                .Data = AuthToken.TokenString

            };

            Params.CredentialsToken.Emplace<UE::Online::FExternalAuthToken>(CredentialsToken);


            OnlineServicesInfo.OnlineServices->GetAuthInterface()->Login(MoveTemp(Params)).OnComplete([]
            (const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& Result)
                {
                    if (Result.IsOk())
                    {
                        check(0)
                        const auto AccountInfo = Result.GetOkValue().
                                                        AccountInfo;
                        if (AccountInfo->AccountId)
                        {
                            UE_LOG(LogTemp, Display, TEXT("Login successfully %d"),
                                   AccountInfo->PlatformUserId.GetInternalId());
                        }
                    }
                    else
                    {
                        check(0)
                    }
                });
        });

    Identity->GetLinkedAccountAuthToken(LocalPlayer->GetLocalPlayerIndex(),
                                        "WebAPI:epiconlineservices",
                                        Delegate);
}
