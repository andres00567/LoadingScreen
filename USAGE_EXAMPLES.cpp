// Example usage of LoadingScreenSubsystem in C++

// Include the subsystem header
#include "LoadingScreenSubsystem.h"

// ============================================================================
// Example 1: Show/Hide Loading Screen Manually
// ============================================================================
void AMyGameMode::ShowLoadingManually()
{
    ULoadingScreenSubsystem* LoadingScreen = GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

    // Show loading screen that must be manually stopped
    LoadingScreen->ShowLoadingScreen(true, 2.0f);

    // Do your work...

    // Hide when ready
    LoadingScreen->HideLoadingScreen();
}

// ============================================================================
// Example 2: Async Asset Loading (Runs on Background Thread)
// ============================================================================
void AMyActor::LoadGameAssets()
{
    ULoadingScreenSubsystem* LoadingScreen = GetWorld()->GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

    // Prepare assets to load
    TArray<FSoftObjectPath> AssetsToLoad;
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/Weapons/Rifle.Rifle")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/Characters/Enemy.Enemy")));
    AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/Maps/Level2.Level2")));

    // Load assets asynchronously with loading screen
    FOnAsyncLoadComplete Callback;
    Callback.BindLambda([this]()
    {
        // This callback is called on the GAME THREAD when loading completes
        UE_LOG(LogTemp, Log, TEXT("All assets loaded successfully!"));
        OnAssetsReady();
    });

    // Loading screen shows automatically and hides when complete
    LoadingScreen->LoadAssetsAsync(AssetsToLoad, Callback);
}

// ============================================================================
// Example 3: Custom Background Task (Heavy Computation)
// ============================================================================
void AMyActor::ProcessHeavyData()
{
    ULoadingScreenSubsystem* LoadingScreen = GetWorld()->GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

    // Execute expensive work on WORKER THREAD (no game thread hitch!)
    LoadingScreen->ExecuteAsyncTaskWithLoadingScreen(
        [this]()
        {
            // This lambda runs on a BACKGROUND THREAD
            // Safe to do expensive operations here

            // Example: Process large data
            for (int32 i = 0; i < 1000000; i++)
            {
                // Complex calculations
                float Result = FMath::Sqrt(i) * FMath::Sin(i);
            }

            // Example: File I/O
            FPlatformProcess::Sleep(2.0f);

            // NOTE: Do NOT call UObject methods here! Not thread-safe!
        },
        FOnCustomTaskComplete::CreateLambda([this]()
        {
            // This callback runs on GAME THREAD
            // Safe to call UObject methods here
            UE_LOG(LogTemp, Log, TEXT("Background task completed!"));
            OnProcessingComplete();
        })
    );
}

// ============================================================================
// Example 4: Listen to Loading Screen Events
// ============================================================================
void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    ULoadingScreenSubsystem* LoadingScreen = GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

    // Bind to show event
    LoadingScreen->OnLoadingScreenShow.AddDynamic(this, &AMyPlayerController::HandleLoadingScreenShow);

    // Bind to hide event
    LoadingScreen->OnLoadingScreenHide.AddDynamic(this, &AMyPlayerController::HandleLoadingScreenHide);
}

void AMyPlayerController::HandleLoadingScreenShow()
{
    // Called when loading screen appears
    // Example: Pause game audio, show cursor, etc.
    UE_LOG(LogTemp, Log, TEXT("Loading screen shown"));
}

void AMyPlayerController::HandleLoadingScreenHide()
{
    // Called when loading screen disappears
    // Example: Resume game audio, hide cursor, etc.
    UE_LOG(LogTemp, Log, TEXT("Loading screen hidden"));
}

// ============================================================================
// Example 5: Load Assets Before Level Transition
// ============================================================================
void AMyGameMode::PrepareNextLevel(FName NextLevelName)
{
    ULoadingScreenSubsystem* LoadingScreen = GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

    // Show loading screen specifically for level load
    LoadingScreen->ShowLoadingScreenForLevelLoad(NextLevelName);

    // The subsystem automatically handles PreLoadMap/PostLoadMap events
    // No need to manually hide the loading screen

    // Load the level
    UGameplayStatics::OpenLevel(this, NextLevelName);
}

// ============================================================================
// Example 6: Chain Multiple Async Operations
// ============================================================================
void AMyGameMode::InitializeMatch()
{
    ULoadingScreenSubsystem* LoadingScreen = GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

    // Show loading screen
    LoadingScreen->ShowLoadingScreen(true, 1.0f);

    // Step 1: Load match assets
    TArray<FSoftObjectPath> MatchAssets;
    MatchAssets.Add(FSoftObjectPath(TEXT("/Game/Maps/Arena.Arena")));

    LoadingScreen->LoadAssetsAsync(MatchAssets, FOnAsyncLoadComplete::CreateLambda([this, LoadingScreen]()
    {
        // Step 2: Process player data on background thread
        LoadingScreen->ExecuteAsyncTaskWithLoadingScreen(
            []()
            {
                // Heavy computation on worker thread
                FPlatformProcess::Sleep(1.0f);
            },
            FOnCustomTaskComplete::CreateLambda([this, LoadingScreen]()
            {
                // Step 3: Load player assets
                TArray<FSoftObjectPath> PlayerAssets;
                PlayerAssets.Add(FSoftObjectPath(TEXT("/Game/Characters/Hero.Hero")));

                LoadingScreen->LoadAssetsAsync(PlayerAssets, FOnAsyncLoadComplete::CreateLambda([this, LoadingScreen]()
                {
                    // All done - hide loading screen
                    LoadingScreen->HideLoadingScreen();
                    StartMatch();
                }));
            })
        );
    }));
}

// ============================================================================
// Example 7: Check Loading Screen Status
// ============================================================================
void AMyHUD::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ULoadingScreenSubsystem* LoadingScreen = GetWorld()->GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

    if (LoadingScreen->IsLoadingScreenVisible())
    {
        // Loading screen is active - maybe hide other UI elements
    }
}

// ============================================================================
// Example 8: Async Load with Progress Tracking (Advanced)
// ============================================================================
void AMyGameMode::LoadWithProgress()
{
    ULoadingScreenSubsystem* LoadingScreen = GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();
    LoadingScreen->ShowLoadingScreen(true, 0.5f);

    TArray<FSoftObjectPath> AssetsToLoad;
    // Add many assets...

    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(
        AssetsToLoad,
        [LoadingScreen]()
        {
            // Complete
            LoadingScreen->HideLoadingScreen();
        }
    );

    // Poll progress on game thread
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [Handle]()
    {
        if (Handle.IsValid())
        {
            float Progress = Handle->GetProgress();
            UE_LOG(LogTemp, Log, TEXT("Loading Progress: %f%%"), Progress * 100.0f);
        }
    }, 0.1f, true);
}
