// Copyright 2019-Present tarnishablec. All Rights Reserved.

#include "TaurosSubsystem.h"

#include <eos_common.h>

#include <dwebble/ffi/index.rs.h>

#include "EOSShared.h"
#include "HttpModule.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"

#include "OnlineSubsystemUtils.h"
#include "Interfaces/IHttpResponse.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineServicesEngineUtils.h"
#include "Wynaut/Misc/WynautMiscHelpers.h"

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

		// TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float DeltaTime)
		// {
		//     return true;
		// }));

		GameInstance->OnLocalPlayerAddedEvent.AddLambda([this](ULocalPlayer* LocalPlayer)
		{
			if (GetGameInstance()->GetFirstGamePlayer() == LocalPlayer)
			{
				const auto OutAccountInfo = MakeShared<TOptional<UE::Online::FAccountInfo>>();

				Eos_ExternalToken_Authorize(
					LocalPlayer,
					"WebAPI:epiconlineservices",
					EOS_EExternalAccountType::EOS_EAT_STEAM,
					EOS_EExternalCredentialType::EOS_ECT_STEAM_SESSION_TICKET,
					OutAccountInfo,
					LocalPlayer
				);
			}
		});

		// auto OpenIdOss = Online::GetSubsystem(GetWorld(), "OpenID");

		// if (OpenIdOss)
		// {
		// 	check(0);
		// }
		// else
		// {
		// 	check(0);
		// }
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

// ... (make sure AsyncToFuture helper is defined above) ...

FVoidCoroutine UTaurosSubsystem::Eos_ExternalToken_Authorize(
	const ULocalPlayer* LocalPlayer,
	const FString& TokenType,
	const EOS_EExternalAccountType AccountType,
	const EOS_EExternalCredentialType CredentialType,
	const TSharedRef<TOptional<UE::Online::FAccountInfo>> AccountInfo,
	UE5Coro::TLatentContext<> _)
{
	const auto Oss = Online::GetSubsystem(GetWorld(), LexToString(AccountType));
	if (!Oss)
	{
		co_return;
	}

	const auto Identity = Oss->GetIdentityInterface();
	if (!Identity)
	{
		co_return;
	}

	// --- Step 1: Await the first async operation (GetLinkedAccountAuthToken) ---
	const auto AuthTokenResult = co_await Wynaut::AsyncToFuture<FExternalAuthToken>(
		// The lambda now directly receives the TSharedPtr
		[&](TSharedPtr<TPromise<FExternalAuthToken>> PromisePtr)
		{
			const auto Delegate = IOnlineIdentity::FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateLambda(
				// You can capture the received PromisePtr directly.
				[PromisePtr](int32, bool bSuccess, const FExternalAuthToken& AuthToken)
				{
					PromisePtr->SetValue(AuthToken);
				});
			Identity->GetLinkedAccountAuthToken(LocalPlayer->GetPlatformUserIndex(), TokenType, Delegate);
		});

	if (!AuthTokenResult.IsValid())
	{
		// Failed to get the auth token
		co_return;
	}

	// --- Step 2: Await the second async operation (Login) ---
	UE::Online::FAuthLogin::Params Params{
		.PlatformUserId = LocalPlayer->GetPlatformUserId(),
		.CredentialsType = UE::Online::LoginCredentialsType::ExternalAuth,
	};
	Params.CredentialsToken.Emplace<UE::Online::FExternalAuthToken>(UE::Online::FExternalAuthToken{
		.Type = LexToString(CredentialType),
		.Data = AuthTokenResult.TokenString
	});

	auto LoginResult = co_await Wynaut::AsyncToFuture<UE::Online::TOnlineResult<UE::Online::FAuthLogin>>(
		// The lambda now receives a TSharedPtr, which is safe to capture in the OnComplete delegate.
		[&, Params = MoveTemp(Params)](
		TSharedPtr<TPromise<UE::Online::TOnlineResult<UE::Online::FAuthLogin>>> PromisePtr) mutable
		{
			OnlineServicesInfo.OnlineServices->GetAuthInterface()->Login(MoveTemp(Params)).OnComplete(
				// Capture the shared pointer. It's copyable and safe for delegates.
				[PromisePtr](const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& Result)
				{
					PromisePtr->SetValue(Result);
				});
		});

	// --- Step 3: Process the final result and return the AccountInfo ---
	if (LoginResult.IsOk())
	{
		*AccountInfo = *LoginResult.GetOkValue().AccountInfo;
		co_return;
	}

	// Login failed
	UE_LOG(LogTemp, Error, TEXT("EOS Login Failed: %s"), *LoginResult.GetErrorValue().GetText().ToString());
	co_return;
}

void UTaurosSubsystem::Eos_OidcAuthCode_Authorize_Old(const ULocalPlayer* LocalPlayer, const FString& Issuer)
{
	auto& Http = FHttpModule::Get();

	const auto Request = Http.CreateRequest();
	Request->SetVerb("GET");
	Request->SetURL(FString::Format(TEXT("{0}/.well-known/openid-configuration"), {Issuer}));

	Request->OnProcessRequestComplete().BindLambda(
		[this](FHttpRequestPtr /*Request*/, const FHttpResponsePtr& Response, const bool bSuccessful)
		{
			if (bSuccessful && Response.IsValid())
			{
				const auto ResponseContent = Response->GetContentAsString();
				TSharedPtr<FJsonObject> JsonObject;
				const auto Reader = TJsonReaderFactory<>::Create(ResponseContent);
				if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
				{
					FString AuthorizationEndpoint;
					if (JsonObject->TryGetStringField(TEXT("authorization_endpoint"), AuthorizationEndpoint))
					{
						if (!Oidc_LoopbackPort.IsSet())
						{
							Oidc_LoopbackPort = dwebble_cxx::port::free_local_ipv4_port();
						}

						constexpr auto Route = "/openid/callback";

						auto Pkce = dwebble_cxx::pkce::pkce_generate();

						// OpenIdStartLoopbackServer(LoopbackPort);

						const auto Router = FHttpServerModule::Get().GetHttpRouter(Oidc_LoopbackPort.GetValue(), true);
						const auto RouteHandle = Router->BindRoute(
							FHttpPath(Route), EHttpServerRequestVerbs::VERB_GET,
							FHttpRequestHandler::CreateLambda(
								[](const FHttpServerRequest& Req, const FHttpResultCallback& OnComplete)
								{
									const auto& QueryParams = Req.QueryParams;
									auto Resp = FHttpServerResponse::Create(FString(""), TEXT("text/plain"));
									OnComplete(MoveTemp(Resp));
									check(0)
									return true;
								}));

						FHttpServerModule::Get().StartAllListeners();

						auto ClientId = TEXT("asuranext");
						auto RedirectUri =
							FString::Format(TEXT("http://127.0.0.1:{0}{1}"), {Oidc_LoopbackPort.GetValue(), Route});
						auto Scope = TEXT("openid profile"); // 常用范围

						const auto AuthUrl = FString::Format(
							TEXT(
								"{0}?response_type=code&client_id={1}&redirect_uri={2}&scope={3}&code_challenge_method={4}&code_challenge={5}&state={6}"),
							{
								AuthorizationEndpoint,
								ClientId,
								RedirectUri,
								Scope,
								Pkce.code_challenge_method.c_str(),
								Pkce.code_challenge.c_str(),
								Pkce.state.c_str()
							});
						FPlatformProcess::LaunchURL(*AuthUrl, nullptr, nullptr);
					}
				}
			}
		});

	Request->ProcessRequest();
}
