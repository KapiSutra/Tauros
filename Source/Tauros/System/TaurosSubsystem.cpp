// Copyright 2019-Present tarnishablec. All Rights Reserved.

#include "TaurosSubsystem.h"

#include <eos_common.h>

#include <dwebble/ffi/index.rs.h>

#include "EOSShared.h"
#include "eos_connect.h"
#include "eos_sdk.h"
#include "HttpModule.h"
#include "HttpServerModule.h"
#include "IEOSSDKManager.h"
#include "IHttpRouter.h"

#include "OnlineSubsystemUtils.h"
#include "Dwebble/Misc/DwebbleMiscLibrary.h"
#include "Interfaces/IHttpResponse.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineServicesCommon.h"
#include "Online/OnlineServicesEngineUtils.h"
#include "Online/OnlineServicesEpicCommon.h"
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


				[](ThisClass* This, ULocalPlayer* InLocalPlayer)-> FVoidCoroutine
				{
					const auto SteamToken = co_await This->Eos_ExternalToken_Oss(
						InLocalPlayer,
						"WebAPI:epiconlineservices",
						EOS_EExternalAccountType::EOS_EAT_STEAM,
						InLocalPlayer
					);

					ensure(SteamToken->IsValid());


					const auto SteamLoginResult = co_await This->Eos_ExternalAuth_Login(
						InLocalPlayer,
						EOS_EExternalCredentialType::EOS_ECT_STEAM_SESSION_TICKET,
						SteamToken.GetValue().TokenString
					);

					check(SteamLoginResult.ResultCode == EOS_EResult::EOS_Success);


					const auto OidcToken = co_await This->Eos_ExternalToken_Oidc(
						TEXT("https://account.punkrain.com/auth/v1"),
						TEXT("asuranext"),
						{TEXT("vxNSrswdmoVPjgXoBZCfsppQfuFlTZWCBJpCMUAHlGLlIGlwftsNaIFlQdLzyOqy")},
						19654,
						TEXT("/openid/callback"),
						InLocalPlayer
					);

					const auto OidcLoginResult = co_await This->Eos_ExternalAuth_Login(
						InLocalPlayer,
						EOS_EExternalCredentialType::EOS_ECT_OPENID_ACCESS_TOKEN,
						OidcToken.GetValue().TokenString
					);

					check(OidcLoginResult.ResultCode == EOS_EResult::EOS_InvalidUser);
				}(this, LocalPlayer);
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

UE5Coro::TCoroutine<TOptional<FExternalAuthToken>> UTaurosSubsystem::Eos_ExternalToken_Oss(
	const ULocalPlayer* LocalPlayer,
	const FString& TokenType,
	const EOS_EExternalAccountType AccountType,
	// const EOS_EExternalCredentialType CredentialType,
	UE5Coro::TLatentContext<> _)
{
	const auto Oss = Online::GetSubsystem(GetWorld(), LexToString(AccountType));
	if (!Oss || !LocalPlayer)
	{
		co_return {};
	}

	const auto Identity = Oss->GetIdentityInterface();
	if (!Identity)
	{
		co_return {};
	}

	// --- Step 1: Await the first async operation (GetLinkedAccountAuthToken) ---
	const auto AuthTokenResult = co_await Wynaut::AsyncToFuture<FExternalAuthToken>(
		// The lambda now directly receives the TSharedPtr
		[&](TSharedPtr<TPromise<FExternalAuthToken>> PromisePtr)
		{
			const auto Delegate = IOnlineIdentity::FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateLambda(
				// You can capture the received PromisePtr directly.
				[PromisePtr](int32, const bool bSuccess, const FExternalAuthToken& AuthToken)
				{
					if (bSuccess)
					{
						PromisePtr->SetValue(AuthToken);
					}
					else
					{
						PromisePtr->SetValue({});
					}
				});
			Identity->GetLinkedAccountAuthToken(LocalPlayer->GetPlatformUserIndex(), TokenType, Delegate);
		});

	if (!AuthTokenResult.IsValid())
	{
		// Failed to get the auth token
		co_return {};
	}

	co_return AuthTokenResult;

#if 0
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
		co_return LoginResult.GetOkValue().AccountInfo.Get();
	}

	// Login failed
	UE_LOG(LogTemp, Error, TEXT("EOS Login Failed: %s"), *LoginResult.GetErrorValue().GetText().ToString());
	co_return {};

#endif
}

UE5Coro::TCoroutine<TOptional<FExternalAuthToken>> UTaurosSubsystem::Eos_ExternalToken_Oidc(
	const FString& Issuer,
	const FString& ClientId,
	const TOptional<FString>& ClientSecret,
	const uint32 LoopbackPort,
	const FString& LoopbackRoute,
	UE5Coro::TLatentContext<> _)
{
	const auto Result = co_await dwebble_cxx::oidc::browser_oidc(
		dwebble_cxx::string::ToRustString(Issuer),
		dwebble_cxx::string::ToRustString(ClientId),
		dwebble_cxx::string::ToRustString(ClientSecret.Get("")),
		LoopbackPort,
		dwebble_cxx::string::ToRustString(LoopbackRoute));

	if (!Result.success)
	{
		co_return {};
	}

	co_return FExternalAuthToken{
		.TokenData = {},
		.TokenString = dwebble_cxx::string::ToFString(Result.access_token)
	};
}

UE5Coro::TCoroutine<EOS_Connect_LoginCallbackInfo> UTaurosSubsystem::Eos_ExternalAuth_Login(
	const ULocalPlayer* LocalPlayer,
	const EOS_EExternalCredentialType ExternalCredentialType,
	const FString Token)
{
	UE::Online::FAuthLogin::Params Params{
		.PlatformUserId = LocalPlayer->GetPlatformUserId(),
		.CredentialsType = UE::Online::LoginCredentialsType::ExternalAuth,
	};
	Params.CredentialsToken.Emplace<UE::Online::FExternalAuthToken>(UE::Online::FExternalAuthToken{
		.Type = LexToString(ExternalCredentialType),
		.Data = Token
	});

	const auto EosService = StaticCastSharedPtr<UE::Online::FOnlineServicesEpicCommon>(UE::Online::GetServices(
		UE::Online::EOnlineServices::Epic));

	if (!EosService)
	{
		co_return {};
	}

	const auto EosPlatformHandle = EosService->GetEOSPlatformHandle();


	if (EosPlatformHandle == nullptr)
	{
		co_return {};
	}

	const auto EosConnectHandle = EOS_Platform_GetConnectInterface(*EosPlatformHandle);

	auto LoginCallbackInfo = co_await Wynaut::AsyncToFuture<const EOS_Connect_LoginCallbackInfo>(
		[&, Params = MoveTemp(Params)](
		TSharedPtr<TPromise<const EOS_Connect_LoginCallbackInfo>> PromisePtr) mutable
		{
			// OnlineServicesInfo.OnlineServices->GetAuthInterface()->Login(MoveTemp(Params)).OnComplete(
			// 	// Capture the shared pointer. It's copyable and safe for delegates.
			// 	[PromisePtr](const UE::Online::TOnlineResult<UE::Online::FAuthLogin>& Result)
			// 	{
			// 		PromisePtr->SetValue(Result);
			// 	});

			struct FEosConnectLoginCtx
			{
				TSharedPtr<TPromise<const EOS_Connect_LoginCallbackInfo>> Promise;
				std::string TokenUtf8;
				EOS_Connect_Credentials Cred{};
				EOS_Connect_LoginOptions Opt{};
			};

			auto* Ctx = new FEosConnectLoginCtx();
			Ctx->Promise = MoveTemp(PromisePtr);

			{
				const auto Utf8 = StringCast<UTF8CHAR>(*Token);
				Ctx->TokenUtf8.assign(
					reinterpret_cast<const char*>(Utf8.Get()),
					Utf8.Length());
			}

			Ctx->Cred.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
			Ctx->Cred.Token = Ctx->TokenUtf8.empty() ? nullptr : Ctx->TokenUtf8.c_str();
			Ctx->Cred.Type = ExternalCredentialType;

			Ctx->Opt.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
			Ctx->Opt.Credentials = &Ctx->Cred;

			EOS_Connect_Login(EosConnectHandle, &Ctx->Opt, Ctx,
			                  [](const EOS_Connect_LoginCallbackInfo* Data)
			                  {
				                  auto* InCtx = static_cast<FEosConnectLoginCtx*>(Data->ClientData);
				                  const EOS_Connect_LoginCallbackInfo Copy = *Data;

				                  AsyncTask(ENamedThreads::GameThread, [InCtx, Copy]()
				                  {
					                  if (InCtx && InCtx->Promise.IsValid())
					                  {
						                  InCtx->Promise->SetValue(Copy);
					                  }
					                  delete InCtx;
				                  });
			                  });
		});

	co_return LoginCallbackInfo;
}

#if 0
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
#endif
