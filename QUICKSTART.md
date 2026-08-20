# Loading Screen Plugin - Quick Start Guide

## What You've Got

A complete **subsystem-based loading screen plugin** for Unreal Engine 5.8 that:

? **Runs off game thread** - Heavy operations execute on background threads  
? **Hooks into Unreal's loading system** - Automatically shows during level loads  
? **Waits for level streaming** - Stays visible until ALL streaming levels are fully loaded  
? **Custom widget support** - Use your own UMG widgets for loading screens  
? **Supports custom async work** - Hide any operation behind a loading screen  
? **Thread-safe** - Proper synchronization for multi-threaded operations  
? **Event-driven** - Broadcast events when screens show/hide  
? **Blueprint & C++ friendly** - Full support for both workflows

## Quick Start

### 1. Configure in Project Settings
1. Open **Project Settings** ? **Plugins** ? **Loading Screen Settings**
2. **[NEW]** Check **Use Custom Widget** and set **Loading Screen Widget Class** to your UMG widget (optional)
3. **[NEW]** **Wait For Level Streaming Complete** is ON by default (recommended)
4. Add your loading screen images to **Loading Images** array (if not using custom widget)
5. Add tips to **Loading Tips** array
6. Adjust **Minimum Loading Screen Display Time** and other timing settings

### 2. Use in Blueprints

**Get the subsystem:**
```
Get Game Instance ? Get Subsystem (Loading Screen Subsystem)
```

**Show loading screen manually:**
```
? Show Loading Screen
    Play Until Stopped: true
    Minimum Display Time: 3.0
```

**Hide when ready:**
```
? Hide Loading Screen
```

**Load assets async:**
```
? Load Assets Async BP
    Assets To Load: [Your Assets Array]
    On Complete: [Your Delegate]
```

### 3. Use in C++

```cpp
// Get subsystem
ULoadingScreenSubsystem* LoadingScreen = 
    GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

// Show/Hide manually
LoadingScreen->ShowLoadingScreen(true, 3.0f);
// ... do work ...
LoadingScreen->HideLoadingScreen();

// Load assets async (on background thread)
TArray<FSoftObjectPath> Assets;
Assets.Add(FSoftObjectPath(TEXT("/Game/MyAsset.MyAsset")));

LoadingScreen->LoadAssetsAsync(Assets, 
    FOnAsyncLoadComplete::CreateLambda([]()
{
    // Called when done
}));

// Run custom background task (no game thread hitch!)
LoadingScreen->ExecuteAsyncTaskWithLoadingScreen(
    []()
    {
        // Runs on WORKER THREAD - do expensive work here
        // DO NOT call UObject methods!
    },
    FOnCustomTaskComplete::CreateLambda([]()
    {
        // Runs on GAME THREAD - safe to call UObject methods
    })
);
```

## How It Works

### Automatic Level Loading
The subsystem hooks into:
- **PreLoadMap**: Shows loading screen before level starts loading
- **PostLoadMap**: Level geometry is loaded, but...
- **[NEW] Level Streaming Check**: Monitors all streaming levels every 0.1s
  - Waits for all streaming levels to be loaded AND visible
  - Checks for pending async loads
  - Only hides when EVERYTHING is complete
- **Hide**: Loading screen disappears after 0.3s delay

**Result**: Players never see partially loaded levels or pop-in!

### Background Thread Execution
- **Asset Loading**: Uses Unreal's StreamableManager (async I/O)
- **Custom Tasks**: Uses AsyncTask on `AnyBackgroundThreadNormalTask`
- **Callbacks**: Always execute on game thread (safe for UObject calls)
- **Task Tracking**: Thread-safe counter tracks active operations

### No Game Thread Hitches
The plugin never blocks the game thread:
- Heavy file I/O happens on I/O threads
- Custom computations run on worker threads
- Only callbacks execute on game thread (which is what you want)

## Key Features

### 1. Manual Control
```cpp
LoadingScreen->ShowLoadingScreen(true, 3.0f);  // Manual mode
// Do work...
LoadingScreen->Hide LoadingScreen();
```

### 2. Async Asset Loading
```cpp
LoadingScreen->LoadAssetsAsync(Assets, Callback);
// Automatically shows/hides, loads on background thread
```

### 3. Custom Background Tasks
```cpp
LoadingScreen->ExecuteAsyncTaskWithLoadingScreen(WorkerThreadLambda, GameThreadCallback);
// Your heavy work runs off-thread!
```

### 4. Events
```cpp
LoadingScreen->OnLoadingScreenShow.AddDynamic(this, &AMyClass::OnShow);
LoadingScreen->OnLoadingScreenHide.AddDynamic(this, &AMyClass::OnHide);
```

### 5. Multiple Concurrent Operations
The subsystem tracks all active operations:
- Multiple asset loads
- Multiple background tasks
- Loading screen stays visible until ALL complete

## Files Created

```
Plugins/LoadingScreen/
??? LoadingScreen.uplugin                      Plugin manifest
??? README.md                                   Full documentation
??? USAGE_EXAMPLES.cpp                         C++ usage examples
??? QUICKSTART.md                              This file
??? Resources/                                  Plugin resources
??? Source/LoadingScreen/
    ??? LoadingScreen.Build.cs                 Build configuration
    ??? Public/
    ?   ??? LoadingScreenModule.h              Module definition
    ?   ??? LoadingScreenSettings.h            Configuration settings
    ?   ??? LoadingScreenSubsystem.h           ?? MAIN SUBSYSTEM
    ?   ??? LoadingScreenWidget.h              Custom widget base class
    ?   ??? LoadingScreenAsyncTask.h           Thread task helper
    ?   ??? AsyncLoadWithLoadingScreen.h       Blueprint async node
    ??? Private/
        ??? LoadingScreenModule.cpp
        ??? LoadingScreenSettings.cpp
        ??? LoadingScreenSubsystem.cpp         ?? MAIN LOGIC
        ??? LoadingScreenWidget.cpp
        ??? AsyncLoadWithLoadingScreen.cpp
```

## Common Use Cases

### Loading Screen for Match Start
```cpp
void AMyGameMode::StartMatch()
{
    LoadingScreen->ShowLoadingScreen(true, 2.0f);

    // Load match assets
    LoadingScreen->LoadAssetsAsync(MatchAssets, 
        FOnAsyncLoadComplete::CreateUObject(this, &AMyGameMode::OnAssetsLoaded));
}

void AMyGameMode::OnAssetsLoaded()
{
    // Initialize players, spawn actors, etc.
    InitializePlayers();

    LoadingScreen->HideLoadingScreen();
}
```

### Procedural Generation Without Hitch
```cpp
void AMyWorldGenerator::GenerateWorld()
{
    LoadingScreen->ExecuteAsyncTaskWithLoadingScreen(
        [this]()
        {
            // Heavy procedural generation on WORKER THREAD
            GenerateTerrainData();      // No hitch!
            GenerateVegetationData();   // No hitch!
            GenerateBuildingData();     // No hitch!
        },
        FOnCustomTaskComplete::CreateUObject(this, &AMyWorldGenerator::SpawnWorld)
    );
}

void AMyWorldGenerator::SpawnWorld()
{
    // Called on GAME THREAD - safe to spawn actors
    SpawnTerrainActors();
    SpawnVegetationActors();
    SpawnBuildingActors();
}
```

### Save Game With Loading Screen
```cpp
void UMySaveGame::SaveGameAsync()
{
    LoadingScreen->ExecuteAsyncTaskWithLoadingScreen(
        [this]()
        {
            // File I/O on WORKER THREAD
            SerializeGameData();
            WriteToFile();
        },
        FOnCustomTaskComplete::CreateLambda([]()
        {
            // Callback on GAME THREAD
            ShowSaveConfirmation();
        })
    );
}
```

## Tips & Best Practices

### ? DO:
- Use `ExecuteAsyncTaskWithLoadingScreen` for heavy computation
- Use `LoadAssetsAsync` for asset loading
- Call UObject methods only in callbacks (game thread)
- Chain operations using callbacks
- Let automatic level loading handle transitions

### ? DON'T:
- Call UObject methods from worker thread lambdas
- Block the game thread with heavy work
- Forget to hide loading screen in manual mode
- Access non-thread-safe data from worker threads

## Need Help?

- **Full Documentation**: See `README.md`
- **C++ Examples**: See `USAGE_EXAMPLES.cpp`
- **Settings**: Project Settings ? Plugins ? Loading Screen Settings

## Threading Model Summary

| Operation | Thread | Use Case |
|-----------|--------|----------|
| `ShowLoadingScreen()` | Game Thread | Manual control |
| `HideLoadingScreen()` | Game Thread | Manual control |
| `LoadAssetsAsync()` | I/O Thread ? Callback on Game Thread | Asset loading |
| `ExecuteAsyncTaskWithLoadingScreen()` | Worker Thread ? Callback on Game Thread | Heavy computation |
| Level loading hooks | Game Thread | Automatic |

**Key Point**: Heavy work runs OFF the game thread. Callbacks run ON the game thread. Perfect!

---

?? **You're all set!** Your game now has a professional loading screen system that doesn't hitch the game thread.
