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
    OnlineServicesInfo->Reset();
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


        GameInstance->OnLocalPlayerAddedEvent.AddLambda([this](const ULocalPlayer* LocalPlayer)
        {
            const auto Oss = Online::GetSubsystem(GetWorld(),"Steam");


            if (!Oss) { return; }

            const auto Identity = Oss->GetIdentityInterface();

            if (!Identity) { return; }

            const auto Delegate = IOnlineIdentity::FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateLambda(
                [LocalPlayer,this](int32 LocalUserNum, bool bWasSuccessful, const FExternalAuthToken& AuthToken)
                {
                    const auto PlatformUserId = LocalPlayer->GetPlatformUserId();

                    UE::Online::FAuthLogin::Params Params{
                        .PlatformUserId = PlatformUserId,
                        .CredentialsType = UE::Online::LoginCredentialsType::ExternalAuth,
                    };


                    UE::Online::FExternalAuthToken CredentialsToken{
                        .Type = LexToString(EOS_EExternalCredentialType::EOS_ECT_STEAM_SESSION_TICKET),
                        .Data = AuthToken.TokenString

                    };

                    Params.CredentialsToken.Emplace<UE::Online::FExternalAuthToken>(CredentialsToken);


                    auto LoginHandle = OnlineServicesInfo->AuthInterface->Login(MoveTemp(Params)).OnComplete(
                        [](const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& Result)
                        {
                            if (Result.IsOk())
                            {
                                check(0)
                                const auto AccountInfo = Result.GetOkValue().AccountInfo;
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

            Identity->GetLinkedAccountAuthToken(0, "WebAPI:epiconlineservices", Delegate);

            // const auto Ticket = Identity->GetAuthToken(0);
        });
    }
}

bool UTaurosSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return Super::ShouldCreateSubsystem(Outer);
}

void UTaurosSubsystem::InitializeOnlineServices()
{
    OnlineServicesInfo = new FOnlineServicesInfo();

    // 初始化服务指针
    OnlineServicesInfo->OnlineServices = UE::Online::GetServices();
    check(OnlineServicesInfo->OnlineServices.IsValid());

    // 验证服务类型
    OnlineServicesInfo->OnlineServicesType = OnlineServicesInfo->OnlineServices->GetServicesProvider();
    if (!OnlineServicesInfo->OnlineServices.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Error: Failed to initialize services."));
        return;
    }

    // 初始化接口指针
    OnlineServicesInfo->AuthInterface = OnlineServicesInfo->OnlineServices->GetAuthInterface();
    OnlineServicesInfo->ConnectivityInterface = OnlineServicesInfo->OnlineServices->GetConnectivityInterface();
}
