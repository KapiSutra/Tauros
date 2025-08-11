// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Online/OnlineServices.h"
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

    void InitializeOnlineServices();

protected:
    FTSTicker::FDelegateHandle TickHandle;

    // EOS
    // UE::Online::TOnlineAsyncOpHandle<UE::Online::FAuthLogin> LoginHandle;


    struct FOnlineServicesInfo
    {
        /** 在线服务指针 - 通过此指针访问接口 */
        UE::Online::IOnlineServicesPtr OnlineServices = nullptr;

        /** Auth接口 */
        UE::Online::IAuthPtr AuthInterface = nullptr;

        /** Connect接口 */
        UE::Online::IConnectivityPtr ConnectivityInterface = nullptr;

        /** 在线服务实现 */
        UE::Online::EOnlineServices OnlineServicesType = UE::Online::EOnlineServices::None;

        /** 将结构体重置为初始设置 */
        void Reset()
        {
            OnlineServices.Reset();
            AuthInterface.Reset();
            OnlineServicesType = UE::Online::EOnlineServices::None;
        }
    };

    FOnlineServicesInfo* OnlineServicesInfo = nullptr;
};
