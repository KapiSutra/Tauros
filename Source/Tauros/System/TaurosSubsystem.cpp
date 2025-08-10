// Copyright 2019-Present tarnishablec. All Rights Reserved.


#include "TaurosSubsystem.h"

#include "OnlineSubsystemUtils.h"


void UTaurosSubsystem::Deinitialize()
{
    Super::Deinitialize();
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
}

void UTaurosSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const auto IsServer = GetGameInstance()->IsDedicatedServerInstance();

    if (!IsServer)
    {
        TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float DeltaTime)
        {
            return true;
        }));

        const auto SteamOss = Online::GetSubsystem(GetWorld(), "Steam");


        if (SteamOss)
        {
            
        }
    }
}

bool UTaurosSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return Super::ShouldCreateSubsystem(Outer);
}
