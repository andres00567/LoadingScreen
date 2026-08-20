#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/StreamableManager.h"
#include "AsyncLoadWithLoadingScreen.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadCompleted);

UCLASS()
class LOADINGSCREEN_API UAsyncLoadWithLoadingScreen : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnLoadCompleted OnCompleted;

    UFUNCTION(BlueprintCallable, Category = "Loading Screen", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
    static UAsyncLoadWithLoadingScreen* LoadAssetsWithLoadingScreen(
        UObject* WorldContextObject,
        const TArray<TSoftObjectPtr<UObject>>& AssetsToLoad,
        bool bShowLoadingScreen = true
    );

    virtual void Activate() override;

private:
    void OnAssetLoadComplete();

    UPROPERTY()
    TObjectPtr<UObject> WorldContextObject;

    TArray<FSoftObjectPath> AssetPaths;
    bool bShouldShowLoadingScreen;
};
