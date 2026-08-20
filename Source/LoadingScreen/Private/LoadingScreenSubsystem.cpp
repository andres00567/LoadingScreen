#include "LoadingScreenSubsystem.h"
#include "LoadingScreenSettings.h"
#include "LoadingScreenAsyncTask.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/LevelStreaming.h"
#include "Engine/GameViewportClient.h"
#include "ContentStreaming.h"
#include "ShaderCompiler.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"
#include "Misc/PackageName.h"
#include "Async/Async.h"
#include "TimerManager.h"
#include "SLoadingScreenProgress.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

class FLoadingScreenInputProcessor final : public IInputProcessor
{
public:
    explicit FLoadingScreenInputProcessor(ULoadingScreenSubsystem* InOwner)
        : Owner(InOwner)
    {
    }

    virtual void Tick(
        const float DeltaTime,
        FSlateApplication& SlateApp,
        TSharedRef<ICursor> Cursor) override
    {
    }

    virtual bool HandleKeyDownEvent(
        FSlateApplication& SlateApp,
        const FKeyEvent& InKeyEvent) override
    {
        const FKey Key = InKeyEvent.GetKey();
        if (Key == EKeys::SpaceBar || Key == EKeys::Enter
            || Key == EKeys::Gamepad_FaceButton_Bottom)
        {
            if (ULoadingScreenSubsystem* Subsystem = Owner.Get())
            {
                return Subsystem->ConfirmTransitionReady();
            }
        }
        return false;
    }

private:
    TWeakObjectPtr<ULoadingScreenSubsystem> Owner;
};

void ULoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bIsLoadingScreenActive = false;
    bManualControl = false;
    bWaitingForLevelStreaming = false;
    bHoldTransitionForInput = false;
    bTransitionReadyForInput = false;
    ConsecutivePostLoadReadyChecks = 0;
    PostLoadReadinessStartTime = 0.0;
    ActiveTaskCount = 0;

    const ULoadingScreenSettings* Settings = GetDefault<ULoadingScreenSettings>();
    bAutomaticLoadingScreensEnabled = !Settings || !Settings->bDelayAutomaticLoadingScreensUntilInitialMap;

    PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ULoadingScreenSubsystem::OnPreLoadMap);
    PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ULoadingScreenSubsystem::OnPostLoadMapWithWorld);
    AsyncLoadingFlushUpdateHandle =
        FCoreDelegates::OnAsyncLoadingFlushUpdate.AddUObject(
            this, &ULoadingScreenSubsystem::HandleAsyncLoadingFlushUpdate);

    SetupLoadingScreen();

    if (FSlateApplication::IsInitialized())
    {
        LoadingInputProcessor =
            MakeShared<FLoadingScreenInputProcessor>(this);
        FSlateApplication::Get().RegisterInputPreProcessor(
            LoadingInputProcessor, 0);
    }
}

void ULoadingScreenSubsystem::Deinitialize()
{
    if (PIEViewportLoadingWidget.IsValid() && GEngine
        && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(
            PIEViewportLoadingWidget.ToSharedRef());
        PIEViewportLoadingWidget.Reset();
    }
    if (LoadingInputProcessor.IsValid()
        && FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().UnregisterInputPreProcessor(
            LoadingInputProcessor);
    }
    LoadingInputProcessor.Reset();
    FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
    FCoreDelegates::OnAsyncLoadingFlushUpdate.Remove(
        AsyncLoadingFlushUpdateHandle);

    if (LevelStreamingCheckTimer.IsValid())
    {
        if (UWorld* World = GetGameInstance()->GetWorld())
        {
            World->GetTimerManager().ClearTimer(LevelStreamingCheckTimer);
        }
    }

    if (InitialMapReadyCheckTimer.IsValid())
    {
        if (UWorld* World = GetGameInstance()->GetWorld())
        {
            World->GetTimerManager().ClearTimer(InitialMapReadyCheckTimer);
        }
    }

    if (TrackedLoadProgressTimer.IsValid())
    {
        if (UWorld* World = GetGameInstance()->GetWorld())
        {
            World->GetTimerManager().ClearTimer(TrackedLoadProgressTimer);
        }
    }

    ActiveHandles.Empty();
    TrackedLoadNames.Empty();

    Super::Deinitialize();
}

void ULoadingScreenSubsystem::SetupLoadingScreen()
{
    if (IsRunningDedicatedServer())
    {
        return;
    }

    const ULoadingScreenSettings* Settings = GetDefault<ULoadingScreenSettings>();
    if (Settings)
    {
        FLoadingScreenAttributes LoadingScreen;
        Settings->SetupLoadingScreen(
            LoadingScreen,
            TransitionStatsTitle,
            TransitionStatsLines);
    }
}

void ULoadingScreenSubsystem::ShowLoadingScreen(bool bPlayUntilStopped, float MinimumDisplayTime)
{
    if (IsRunningDedicatedServer())
    {
        return;
    }

    bManualControl = bPlayUntilStopped;
    bIsLoadingScreenActive = true;

    const ULoadingScreenSettings* Settings = GetDefault<ULoadingScreenSettings>();
    FLoadingScreenAttributes LoadingScreen;

    if (Settings)
    {
        Settings->SetupLoadingScreen(
            LoadingScreen,
            TransitionStatsTitle,
            TransitionStatsLines);
    }
    MoviePlayerLoadingWidget = LoadingScreen.WidgetLoadingScreen;

    LoadingScreen.bAutoCompleteWhenLoadingCompletes = !bPlayUntilStopped;
    LoadingScreen.bWaitForManualStop = bPlayUntilStopped;
    LoadingScreen.MinimumLoadingScreenDisplayTime = MinimumDisplayTime;
    LoadingScreen.bMoviesAreSkippable = false;

    bWaitingForLevelStreaming = Settings
        ? Settings->bWaitForLevelStreamingComplete : true;

    const UWorld* CurrentWorld = GetGameInstance()
        ? GetGameInstance()->GetWorld() : nullptr;
    // Manual screens are displayed before their async request is queued.
    // MoviePlayer manual-wait blocks the game thread, which deadlocks async
    // loading when AsyncLoading2 is running without its worker thread.
    if (bPlayUntilStopped && CurrentWorld
        && LoadingScreen.WidgetLoadingScreen.IsValid()
        && GEngine && GEngine->GameViewport)
    {
        if (PIEViewportLoadingWidget.IsValid())
        {
            GEngine->GameViewport->RemoveViewportWidgetContent(
                PIEViewportLoadingWidget.ToSharedRef());
        }
        PIEViewportLoadingWidget = LoadingScreen.WidgetLoadingScreen;
        GEngine->GameViewport->AddViewportWidgetContent(
            PIEViewportLoadingWidget.ToSharedRef(), 10000);
        OnLoadingScreenShow.Broadcast();
        return;
    }

    GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
    GetMoviePlayer()->PlayMovie();

    OnLoadingScreenShow.Broadcast();
}

void ULoadingScreenSubsystem::HideLoadingScreen()
{
    if (IsRunningDedicatedServer() || !bIsLoadingScreenActive)
    {
        return;
    }

    FLoadingScreenProgressState::Complete();
    if (PIEViewportLoadingWidget.IsValid() && GEngine
        && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(
            PIEViewportLoadingWidget.ToSharedRef());
        PIEViewportLoadingWidget.Reset();
    }
    if (GetMoviePlayer()->IsMovieCurrentlyPlaying())
    {
        GetMoviePlayer()->StopMovie();
    }
    MoviePlayerLoadingWidget.Reset();
    bIsLoadingScreenActive = false;
    bManualControl = false;

    OnLoadingScreenHide.Broadcast();
    ClearTransitionStats();
}

bool ULoadingScreenSubsystem::IsLoadingScreenVisible() const
{
    if (IsRunningDedicatedServer())
    {
        return false;
    }

    return PIEViewportLoadingWidget.IsValid()
        || GetMoviePlayer()->IsMovieCurrentlyPlaying();
}

void ULoadingScreenSubsystem::LoadAssetsAsyncBP(const TArray<FSoftObjectPath>& AssetsToLoad, FOnAsyncLoadCompleteDynamic OnComplete)
{
    if (AssetsToLoad.Num() == 0)
    {
        OnComplete.ExecuteIfBound();
        return;
    }

    ShowLoadingScreen(true, 0.5f);

    TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(
        AssetsToLoad,
        FStreamableDelegate::CreateUObject(this, &ULoadingScreenSubsystem::HandleAsyncLoadCompleteDynamic, Handle, OnComplete),
        FStreamableManager::AsyncLoadHighPriority
    );

    if (Handle.IsValid())
    {
        ActiveHandles.Add(Handle);
    }
}

void ULoadingScreenSubsystem::LoadAssetsAsync(const TArray<FSoftObjectPath>& AssetsToLoad, FOnAsyncLoadComplete OnComplete)
{
    if (AssetsToLoad.Num() == 0)
    {
        OnComplete.ExecuteIfBound();
        return;
    }

    ShowLoadingScreen(true, 0.5f);

    TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(
        AssetsToLoad,
        FStreamableDelegate::CreateUObject(this, &ULoadingScreenSubsystem::HandleAsyncLoadComplete, Handle, OnComplete),
        FStreamableManager::AsyncLoadHighPriority
    );

    if (Handle.IsValid())
    {
        ActiveHandles.Add(Handle);
    }
}

TSharedPtr<FStreamableHandle>
ULoadingScreenSubsystem::RequestAsyncLoadWithProgress(
    const TArray<FSoftObjectPath>& AssetsToLoad,
    const FString& DisplayName,
    FStreamableDelegateWithHandle OnComplete,
    TAsyncLoadPriority Priority)
{
    if (AssetsToLoad.IsEmpty())
    {
        OnComplete.ExecuteIfBound(nullptr);
        return nullptr;
    }

    FLoadingScreenProgressState::SetPhase(
        FString::Printf(TEXT("Loading %s..."), *DisplayName), 0.0f);
    FLoadingScreenProgressState::SetDetails(
        TEXT("Async asset request queued"));

    FStreamableAsyncLoadParams Params;
    Params.TargetsToStream = AssetsToLoad;
    Params.Priority = Priority;
    Params.OnUpdate = FStreamableUpdateDelegate::CreateUObject(
        this,
        &ULoadingScreenSubsystem::HandleTrackedLoadUpdate,
        DisplayName);
    Params.OnComplete = FStreamableDelegateWithHandle::CreateUObject(
        this,
        &ULoadingScreenSubsystem::HandleTrackedLoadComplete,
        MoveTemp(OnComplete),
        DisplayName);

    TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(
        MoveTemp(Params),
        FString::Printf(TEXT("Loading screen: %s"), *DisplayName));
    if (Handle.IsValid())
    {
        ActiveHandles.Add(Handle);
        TrackedLoadNames.Add(Handle, DisplayName);
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                TrackedLoadProgressTimer,
                this,
                &ULoadingScreenSubsystem::PollTrackedLoadProgress,
                0.05f,
                true);
        }
        HandleTrackedLoadUpdate(Handle.ToSharedRef(), DisplayName);
        UE_LOG(LogTemp, Display,
            TEXT("LoadingScreen: queued tracked async load name=%s targets=%d"),
            *DisplayName, AssetsToLoad.Num());
    }
    return Handle;
}

void ULoadingScreenSubsystem::HandleTrackedLoadUpdate(
    TSharedRef<FStreamableHandle> Handle,
    FString DisplayName)
{
    const float Progress = FMath::Clamp(
        Handle->GetProgress(), 0.0f, 1.0f);
    FLoadingScreenProgressState::SetPhase(
        FString::Printf(TEXT("Loading %s..."), *DisplayName), Progress);
    FLoadingScreenProgressState::SetDetails(FString::Printf(
        TEXT("Async asset request: %.1f%% complete"), Progress * 100.0f));
}

void ULoadingScreenSubsystem::HandleTrackedLoadComplete(
    TSharedPtr<FStreamableHandle> Handle,
    FStreamableDelegateWithHandle Callback,
    FString DisplayName)
{
    ActiveHandles.Remove(Handle);
    TrackedLoadNames.Remove(Handle);
    if (TrackedLoadNames.IsEmpty())
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(TrackedLoadProgressTimer);
        }
    }
    FLoadingScreenProgressState::SetIndeterminatePhase(
        FString::Printf(TEXT("Activating %s..."), *DisplayName));
    FLoadingScreenProgressState::SetDetails(
        TEXT("Async asset request complete; activating destination world"));
    UE_LOG(LogTemp, Display,
        TEXT("LoadingScreen: tracked async load complete name=%s valid=%d error=%d"),
        *DisplayName, Handle.IsValid(),
        Handle.IsValid() && Handle->HasError());
    Callback.ExecuteIfBound(Handle);
}

void ULoadingScreenSubsystem::PollTrackedLoadProgress()
{
    for (const TPair<TSharedPtr<FStreamableHandle>, FString>& Entry
         : TrackedLoadNames)
    {
        if (Entry.Key.IsValid() && Entry.Key->IsLoadingInProgress())
        {
            HandleTrackedLoadUpdate(Entry.Key.ToSharedRef(), Entry.Value);
        }
    }
}

void ULoadingScreenSubsystem::HandleAsyncLoadComplete(TSharedPtr<FStreamableHandle> Handle, FOnAsyncLoadComplete Callback)
{
    ActiveHandles.Remove(Handle);

    Callback.ExecuteIfBound();

    if (!bManualControl && ActiveHandles.Num() == 0 && ActiveTaskCount == 0)
    {
        HideLoadingScreen();
    }
}

void ULoadingScreenSubsystem::HandleAsyncLoadCompleteDynamic(TSharedPtr<FStreamableHandle> Handle, FOnAsyncLoadCompleteDynamic Callback)
{
    ActiveHandles.Remove(Handle);

    Callback.ExecuteIfBound();

    if (!bManualControl && ActiveHandles.Num() == 0 && ActiveTaskCount == 0)
    {
        HideLoadingScreen();
    }
}

void ULoadingScreenSubsystem::ExecuteAsyncTaskWithLoadingScreen(TFunction<void()> Task, FOnCustomTaskComplete OnComplete)
{
    ShowLoadingScreen(true, 0.5f);

    {
        FScopeLock Lock(&TaskCountLock);
        ActiveTaskCount++;
    }

    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Task, OnComplete]()
    {
        if (Task)
        {
            Task();
        }

        AsyncTask(ENamedThreads::GameThread, [this, OnComplete]()
        {
            HandleCustomTaskComplete(OnComplete);
        });
    });
}

void ULoadingScreenSubsystem::HandleCustomTaskComplete(FOnCustomTaskComplete Callback)
{
    {
        FScopeLock Lock(&TaskCountLock);
        ActiveTaskCount = FMath::Max(0, ActiveTaskCount - 1);
    }

    Callback.ExecuteIfBound();

    if (!bManualControl && ActiveHandles.Num() == 0 && ActiveTaskCount == 0)
    {
        HideLoadingScreen();
    }
}

void ULoadingScreenSubsystem::ShowLoadingScreenForLevelLoad(FName LevelName)
{
    ShowLoadingScreen(false, 2.0f);
}

void ULoadingScreenSubsystem::SetTransitionStats(
    const FString& Title,
    const TArray<FString>& StatLines)
{
    TransitionStatsTitle = Title;
    TransitionStatsLines = StatLines;
    // Campaign transitions remain visible after all requested content is
    // ready and explicitly tell the player how to continue.
    bHoldTransitionForInput = true;
    bTransitionReadyForInput = false;
}

void ULoadingScreenSubsystem::ClearTransitionStats()
{
    TransitionStatsTitle.Reset();
    TransitionStatsLines.Reset();
    bHoldTransitionForInput = false;
    bTransitionReadyForInput = false;
}

bool ULoadingScreenSubsystem::ConfirmTransitionReady()
{
    if (bHoldTransitionForInput && bTransitionReadyForInput)
    {
        HideLoadingScreen();
        return true;
    }
    return false;
}

void ULoadingScreenSubsystem::OnPreLoadMap(const FString& MapName)
{
    ActiveMapLoadName = FPackageName::GetShortName(MapName);
    if (!TransitionStatsLines.IsEmpty())
    {
        // Blocking world activation does not expose a trustworthy percentage.
        // Preserve an honest marquee rather than displaying a fake 90-95% hold.
        FLoadingScreenProgressState::SetIndeterminatePhase(
            FString::Printf(
                TEXT("Activating %s..."),
                *FPackageName::GetShortName(MapName)));
    }
    else
    {
        FLoadingScreenProgressState::BeginMapLoad(MapName);
    }
    // Startup suppression is only for ordinary automatic map loads. A
    // campaign segment transition has explicitly supplied results and must
    // still appear when PIE was launched directly into a campaign map (where
    // the configured initial menu map was never observed).
    if (!bAutomaticLoadingScreensEnabled && !bHoldTransitionForInput)
    {
        UE_LOG(LogTemp, Verbose, TEXT("LoadingScreen: Suppressing automatic screen during startup map load: %s"), *MapName);
        return;
    }

    if (!bManualControl)
    {
        const ULoadingScreenSettings* Settings = GetDefault<ULoadingScreenSettings>();
        ShowLoadingScreen(
            bHoldTransitionForInput,
            Settings ? Settings->MinimumLoadingScreenDisplayTime : 1.0f);
        bWaitingForLevelStreaming = Settings ? Settings->bWaitForLevelStreamingComplete : true;
    }
}

void ULoadingScreenSubsystem::HandleAsyncLoadingFlushUpdate()
{
    if (!bIsLoadingScreenActive)
    {
        return;
    }

    const int32 ShaderJobs = GShaderCompilingManager
        ? GShaderCompilingManager->GetNumRemainingJobs() : 0;
    if (ShaderJobs > 0)
    {
        FLoadingScreenProgressState::SetDetails(FString::Printf(
            TEXT("Compiling shaders: %d jobs remaining"), ShaderJobs));
    }
    else
    {
        FLoadingScreenProgressState::SetDetails(FString::Printf(
            TEXT("Engine package loader is activating %s"),
            ActiveMapLoadName.IsEmpty()
                ? TEXT("the destination world") : *ActiveMapLoadName));
    }
}

void ULoadingScreenSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
    ConsecutivePostLoadReadyChecks = 0;
    PostLoadReadinessStartTime = FPlatformTime::Seconds();
    FLoadingScreenProgressState::SetIndeterminatePhase(
        TEXT("Preparing world..."));
    FLoadingScreenProgressState::SetDetails(
        TEXT("Persistent world loaded; checking shaders and streamed content"));
    const ULoadingScreenSettings* Settings = GetDefault<ULoadingScreenSettings>();

    // MoviePlayer is ideal while travel blocks the game thread, but an
    // automatic movie screen is allowed to disappear as soon as package load
    // completes. Hand the same Slate tree to the live viewport before that
    // happens so post-load streaming and the first cinematic camera cut remain
    // continuously covered.
    if (bIsLoadingScreenActive && !PIEViewportLoadingWidget.IsValid()
        && MoviePlayerLoadingWidget.IsValid() && GEngine
        && GEngine->GameViewport)
    {
        // Detach it from MoviePlayer before assigning the Slate widget a new
        // viewport parent. Both operations occur in this post-load callback,
        // before the destination world can present a frame.
        if (GetMoviePlayer()->IsMovieCurrentlyPlaying())
        {
            GetMoviePlayer()->StopMovie();
        }
        PIEViewportLoadingWidget = MoviePlayerLoadingWidget;
        GEngine->GameViewport->AddViewportWidgetContent(
            PIEViewportLoadingWidget.ToSharedRef(), 10000);
    }

    if (!bAutomaticLoadingScreensEnabled && Settings && LoadedWorld)
    {
        const FString LoadedMapName = FPackageName::GetShortName(LoadedWorld->GetOutermost()->GetName());
        if (LoadedMapName.Equals(Settings->InitialMapName.ToString(), ESearchCase::IgnoreCase))
        {
            if (Settings->bWaitForLevelStreamingComplete)
            {
                LoadedWorld->GetTimerManager().SetTimer(
                    InitialMapReadyCheckTimer,
                    FTimerDelegate::CreateUObject(this, &ULoadingScreenSubsystem::CheckInitialMapReady, LoadedWorld),
                    0.1f,
                    true
                );
            }
            else
            {
                bAutomaticLoadingScreensEnabled = true;
                UE_LOG(LogTemp, Log, TEXT("LoadingScreen: Startup map %s is ready; automatic loading screens enabled."), *LoadedMapName);
            }
        }
    }

    if (bIsLoadingScreenActive && LoadedWorld)
    {
        if (Settings && Settings->bWaitForLevelStreamingComplete && bWaitingForLevelStreaming)
        {
            // Start checking for level streaming completion
            LoadedWorld->GetTimerManager().SetTimer(
                LevelStreamingCheckTimer,
                FTimerDelegate::CreateUObject(this, &ULoadingScreenSubsystem::CheckLevelStreamingComplete, LoadedWorld),
                0.1f,
                true
            );
        }
        else
        {
            if (bHoldTransitionForInput)
            {
                MarkTransitionReady();
            }
            else
            {
                HideLoadingScreen();
            }
        }
    }
}

void ULoadingScreenSubsystem::CheckLevelStreamingComplete(UWorld* World)
{
    if (!World || !bIsLoadingScreenActive)
    {
        if (LevelStreamingCheckTimer.IsValid() && World)
        {
            World->GetTimerManager().ClearTimer(LevelStreamingCheckTimer);
        }
        return;
    }

    const ULoadingScreenSettings* Settings =
        GetDefault<ULoadingScreenSettings>();

    int32 RequestedLevels = 0;
    int32 ReadyLevels = 0;
    FString PendingLevelName;
    for (const ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
    {
        if (StreamingLevel && StreamingLevel->ShouldBeLoaded())
        {
            ++RequestedLevels;
            if (StreamingLevel->IsLevelLoaded() && StreamingLevel->IsLevelVisible())
            {
                ++ReadyLevels;
            }
            else if (PendingLevelName.IsEmpty())
            {
                PendingLevelName = FPackageName::GetShortName(
                    StreamingLevel->GetWorldAssetPackageName());
            }
        }
    }
    const int32 ShaderJobs = GShaderCompilingManager
        ? GShaderCompilingManager->GetNumRemainingJobs() : 0;
    const int32 StreamingAssets =
        IStreamingManager::Get().GetNumWantingResources();
    FLoadingScreenProgressState::SetDetails(FString::Printf(
        TEXT("Shaders: %d remaining  |  Levels: %d / %d  |  Streaming assets: %d pending"),
        ShaderJobs, ReadyLevels, RequestedLevels, StreamingAssets));

    if (RequestedLevels > 0)
    {
        const float StreamingAlpha =
            static_cast<float>(ReadyLevels) / static_cast<float>(RequestedLevels);
        FLoadingScreenProgressState::SetPhase(
            PendingLevelName.IsEmpty()
                ? FString::Printf(
                    TEXT("Finalizing streamed levels... %d / %d"),
                    ReadyLevels, RequestedLevels)
                : FString::Printf(
                    TEXT("Streaming %s... %d / %d"),
                    *PendingLevelName, ReadyLevels, RequestedLevels),
            StreamingAlpha);
    }
    else if (ShaderJobs > 0)
    {
        FLoadingScreenProgressState::SetIndeterminatePhase(FString::Printf(
            TEXT("Compiling shaders: %d remaining"), ShaderJobs));
    }
    else if (StreamingAssets > 0)
    {
        FLoadingScreenProgressState::SetIndeterminatePhase(FString::Printf(
            TEXT("Streaming assets: %d pending"), StreamingAssets));
    }

    const bool bReadyThisCheck = IsWorldFullyLoaded(World);
    ConsecutivePostLoadReadyChecks = bReadyThisCheck
        ? ConsecutivePostLoadReadyChecks + 1
        : 0;
    const int32 RequiredStableChecks = Settings
        ? FMath::Max(1, Settings->PostLoadStableCheckCount)
        : 2;
    const double ReadinessElapsed = FPlatformTime::Seconds()
        - PostLoadReadinessStartTime;
    const bool bReadinessTimedOut = Settings
        && Settings->MaximumPostLoadAssetWaitTime > 0.0f
        && ReadinessElapsed >= Settings->MaximumPostLoadAssetWaitTime;

    if (ConsecutivePostLoadReadyChecks >= RequiredStableChecks
        || bReadinessTimedOut)
    {
        if (bReadinessTimedOut && !bReadyThisCheck)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("LoadingScreen: post-load readiness timed out after %.2fs (shaders=%d streamingAssets=%d); releasing transition"),
                ReadinessElapsed, ShaderJobs, StreamingAssets);
        }
        World->GetTimerManager().ClearTimer(LevelStreamingCheckTimer);
        bWaitingForLevelStreaming = false;

        if (bHoldTransitionForInput)
        {
            MarkTransitionReady();
        }
        else
        {
            HideLoadingScreen();
        }
    }
}

void ULoadingScreenSubsystem::MarkTransitionReady()
{
    if (!bHoldTransitionForInput || !bIsLoadingScreenActive)
    {
        return;
    }

    bTransitionReadyForInput = true;
    FLoadingScreenProgressState::SetPhase(
        TEXT("READY - PRESS JUMP TO CONTINUE"), 1.0f);
    FLoadingScreenProgressState::SetDetails(
        TEXT("Space / Enter / Gamepad Confirm"));
    UE_LOG(LogTemp, Display,
        TEXT("LoadingScreen: destination ready; waiting for Jump input"));
}

void ULoadingScreenSubsystem::CheckInitialMapReady(UWorld* World)
{
    if (!World || bAutomaticLoadingScreensEnabled)
    {
        if (World)
        {
            World->GetTimerManager().ClearTimer(InitialMapReadyCheckTimer);
        }
        return;
    }

    if (IsWorldFullyLoaded(World))
    {
        World->GetTimerManager().ClearTimer(InitialMapReadyCheckTimer);
        bAutomaticLoadingScreensEnabled = true;

        UE_LOG(LogTemp, Log, TEXT("LoadingScreen: Startup map %s is fully loaded; automatic loading screens enabled."),
            *FPackageName::GetShortName(World->GetOutermost()->GetName()));
    }
}

bool ULoadingScreenSubsystem::IsWorldFullyLoaded(const UWorld* World) const
{
    if (!World)
    {
        return false;
    }

    for (const ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
    {
        if (StreamingLevel && StreamingLevel->ShouldBeLoaded()
            && (!StreamingLevel->IsLevelLoaded() || !StreamingLevel->IsLevelVisible()))
        {
            return false;
        }
    }

    const int32 ShaderJobs = GShaderCompilingManager
        ? GShaderCompilingManager->GetNumRemainingJobs() : 0;
    if (ShaderJobs > 0 || IStreamingManager::Get().GetNumWantingResources() > 0)
    {
        return false;
    }

    // The stable-check window in CheckLevelStreamingComplete prevents a
    // transient zero from exposing the destination between streaming bursts.
    return true;
}
