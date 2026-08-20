#include "AsyncLoadWithLoadingScreen.h"
#include "LoadingScreenSubsystem.h"
#include "Engine/GameInstance.h"

UAsyncLoadWithLoadingScreen* UAsyncLoadWithLoadingScreen::LoadAssetsWithLoadingScreen(
    UObject* WorldContextObject,
    const TArray<TSoftObjectPtr<UObject>>& AssetsToLoad,
    bool bShowLoadingScreen)
{
    UAsyncLoadWithLoadingScreen* Node = NewObject<UAsyncLoadWithLoadingScreen>();
    Node->WorldContextObject = WorldContextObject;
    Node->bShouldShowLoadingScreen = bShowLoadingScreen;

    for (const TSoftObjectPtr<UObject>& Asset : AssetsToLoad)
    {
        Node->AssetPaths.Add(Asset.ToSoftObjectPath());
    }

    return Node;
}

void UAsyncLoadWithLoadingScreen::Activate()
{
    if (!WorldContextObject)
    {
        OnCompleted.Broadcast();
        return;
    }

    UGameInstance* GameInstance = WorldContextObject->GetWorld() ? WorldContextObject->GetWorld()->GetGameInstance() : nullptr;
    if (!GameInstance)
    {
        OnCompleted.Broadcast();
        return;
    }

    ULoadingScreenSubsystem* LoadingScreenSubsystem = GameInstance->GetSubsystem<ULoadingScreenSubsystem>();
    if (!LoadingScreenSubsystem)
    {
        OnCompleted.Broadcast();
        return;
    }

    if (bShouldShowLoadingScreen)
    {
        FOnAsyncLoadComplete Delegate;
        Delegate.BindUObject(this, &UAsyncLoadWithLoadingScreen::OnAssetLoadComplete);
        LoadingScreenSubsystem->LoadAssetsAsync(AssetPaths, Delegate);
    }
    else
    {
        OnAssetLoadComplete();
    }
}

void UAsyncLoadWithLoadingScreen::OnAssetLoadComplete()
{
    OnCompleted.Broadcast();
    SetReadyToDestroy();
}
