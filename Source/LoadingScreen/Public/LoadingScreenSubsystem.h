#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/StreamableManager.h"
#include "LoadingScreenSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadingScreenShow);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadingScreenHide);
DECLARE_DYNAMIC_DELEGATE(FOnAsyncLoadCompleteDynamic);

DECLARE_DELEGATE(FOnAsyncLoadComplete);
DECLARE_DELEGATE(FOnCustomTaskComplete);

class FLoadingScreenInputProcessor;
class SWidget;

UCLASS()
class LOADINGSCREEN_API ULoadingScreenSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Loading Screen")
    void ShowLoadingScreen(bool bPlayUntilStopped = false, float MinimumDisplayTime = 3.0f);

    UFUNCTION(BlueprintCallable, Category = "Loading Screen")
    void HideLoadingScreen();

    UFUNCTION(BlueprintPure, Category = "Loading Screen")
    bool IsLoadingScreenVisible() const;

    UFUNCTION(BlueprintCallable, Category = "Loading Screen")
    void LoadAssetsAsyncBP(const TArray<FSoftObjectPath>& AssetsToLoad, FOnAsyncLoadCompleteDynamic OnComplete);

    void LoadAssetsAsync(const TArray<FSoftObjectPath>& AssetsToLoad, FOnAsyncLoadComplete OnComplete);

    /** Starts and reports one measured streamable request for a caller-owned transition. */
    TSharedPtr<FStreamableHandle> RequestAsyncLoadWithProgress(
        const TArray<FSoftObjectPath>& AssetsToLoad,
        const FString& DisplayName,
        FStreamableDelegateWithHandle OnComplete,
        TAsyncLoadPriority Priority = FStreamableManager::AsyncLoadHighPriority);

    UFUNCTION(BlueprintCallable, Category = "Loading Screen")
    void ShowLoadingScreenForLevelLoad(FName LevelName);

    /** Adds the completed segment's stats to the next map loading screen. */
    UFUNCTION(BlueprintCallable, Category = "Loading Screen|Transition")
    void SetTransitionStats(
        const FString& Title,
        const TArray<FString>& StatLines);

    UFUNCTION(BlueprintCallable, Category = "Loading Screen|Transition")
    void ClearTransitionStats();

    /** Releases a fully loaded between-segment screen after explicit player input. */
    bool ConfirmTransitionReady();

    void ExecuteAsyncTaskWithLoadingScreen(TFunction<void()> Task, FOnCustomTaskComplete OnComplete);

    UPROPERTY(BlueprintAssignable, Category = "Loading Screen")
    FOnLoadingScreenShow OnLoadingScreenShow;

    UPROPERTY(BlueprintAssignable, Category = "Loading Screen")
    FOnLoadingScreenHide OnLoadingScreenHide;

private:
    void SetupLoadingScreen();
    void OnPreLoadMap(const FString& MapName);
    void OnPostLoadMapWithWorld(UWorld* LoadedWorld);
    void HandleAsyncLoadingFlushUpdate();
    void CheckLevelStreamingComplete(UWorld* World);
    void CheckInitialMapReady(UWorld* World);
    void MarkTransitionReady();
    bool IsWorldFullyLoaded(const UWorld* World) const;

    void HandleAsyncLoadComplete(TSharedPtr<FStreamableHandle> Handle, FOnAsyncLoadComplete Callback);
    void HandleAsyncLoadCompleteDynamic(TSharedPtr<FStreamableHandle> Handle, FOnAsyncLoadCompleteDynamic Callback);
    void HandleTrackedLoadUpdate(
        TSharedRef<FStreamableHandle> Handle,
        FString DisplayName);
    void HandleTrackedLoadComplete(
        TSharedPtr<FStreamableHandle> Handle,
        FStreamableDelegateWithHandle Callback,
        FString DisplayName);
    void PollTrackedLoadProgress();
    void HandleCustomTaskComplete(FOnCustomTaskComplete Callback);

    FDelegateHandle PreLoadMapHandle;
    FDelegateHandle PostLoadMapHandle;
    FDelegateHandle AsyncLoadingFlushUpdateHandle;

    bool bIsLoadingScreenActive;
    bool bManualControl;
    bool bWaitingForLevelStreaming;
    bool bAutomaticLoadingScreensEnabled;
    bool bHoldTransitionForInput;
    bool bTransitionReadyForInput;
    int32 ConsecutivePostLoadReadyChecks;
    double PostLoadReadinessStartTime;
    int32 ActiveTaskCount;

    FStreamableManager StreamableManager;
    TArray<TSharedPtr<FStreamableHandle>> ActiveHandles;
    FTimerHandle LevelStreamingCheckTimer;
    FTimerHandle InitialMapReadyCheckTimer;
    FTimerHandle TrackedLoadProgressTimer;
    TMap<TSharedPtr<FStreamableHandle>, FString> TrackedLoadNames;

    TSharedPtr<FLoadingScreenInputProcessor> LoadingInputProcessor;
    TSharedPtr<SWidget> PIEViewportLoadingWidget;
    TSharedPtr<SWidget> MoviePlayerLoadingWidget;

    FCriticalSection TaskCountLock;

    FString TransitionStatsTitle;
    TArray<FString> TransitionStatsLines;
    FString ActiveMapLoadName;
};
