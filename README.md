# Loading Screen Plugin

A subsystem-based loading screen plugin for Unreal Engine 5.8 that hooks into engine loading events and supports custom async operations on background threads.

## Features

- **Subsystem Architecture**: Lives as a Game Instance Subsystem
- **Automatic Level Loading Detection**: Hooks into PreLoadMap and PostLoadMap events
- **Async Asset Loading**: Load assets on background threads with automatic loading screen
- **Custom Background Tasks**: Execute any code on worker threads with loading screen overlay
- **Thread-Safe**: Uses proper locking mechanisms for multi-threaded operations
- **Blueprint & C++ Support**: Full support for both workflows
- **Event-Driven**: Broadcast events when loading screens show/hide
- **Customizable**: Configure images, tips, timing, and behavior

## Architecture

The plugin uses a Game Instance Subsystem that:
- Automatically hooks into Unreal's level streaming system
- Manages async asset loading via StreamableManager
- Executes custom tasks on background threads using AsyncTask
- Tracks active operations and automatically hides loading screen when complete
- Does not hitch the game thread

## Setup

1. The plugin is automatically enabled in your project
2. Access the subsystem via: `UGameInstance::GetSubsystem<ULoadingScreenSubsystem>()`
3. Configure settings in **Project Settings > Plugins > Loading Screen Settings**

## Configuration

### Loading Images
- Navigate to **Project Settings > Plugins > Loading Screen Settings**
- Add texture references to **Loading Images** array
- Random image selected each time

### Loading Tips
- Edit **Loading Tips** array
- Tips display at bottom of screen
- Random tip selected each time

### Movies (Optional)
- Add movie file paths to **Startup Movies**
- Supported: MP4, WebM

## Usage

### Automatic Level Loading (Built-in)
The subsystem automatically shows loading screens during level transitions. No code needed!

### Blueprint - Show/Hide Manually
```
Get Game Instance -> Get Subsystem (Loading Screen Subsystem)
  -> Show Loading Screen (Play Until Stopped: true, Minimum Display Time: 3.0)

  // Do your work...

  -> Hide Loading Screen
```

### Blueprint - Async Asset Loading
```
Get Game Instance -> Get Subsystem (Loading Screen Subsystem)
  -> Load Assets Async (Assets To Load: [Asset1, Asset2...])

  // Loading screen automatically shows and hides
```

### Blueprint - Using Async Action Node
```
Load Assets With Loading Screen
  - Assets To Load: [Your Assets]
  - Show Loading Screen: true
  -> On Completed
```

### C++ - Manual Control
```cpp
ULoadingScreenSubsystem* LoadingScreen = GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

// Show loading screen
LoadingScreen->ShowLoadingScreen(true, 3.0f);

// Your work here...

// Hide when done
LoadingScreen->HideLoadingScreen();
```

### C++ - Async Asset Loading (Background Thread)
```cpp
ULoadingScreenSubsystem* LoadingScreen = GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

TArray<FSoftObjectPath> AssetsToLoad;
AssetsToLoad.Add(FSoftObjectPath(TEXT("/Game/MyAsset.MyAsset")));

FOnAsyncLoadComplete Callback;
Callback.BindLambda([this]()
{
    // Called on game thread when loading complete
    UE_LOG(LogTemp, Log, TEXT("Assets loaded!"));
});

LoadingScreen->LoadAssetsAsync(AssetsToLoad, Callback);
// Loading screen shows automatically, hides when complete
```

### C++ - Custom Background Task (No Game Thread Hitch)
```cpp
ULoadingScreenSubsystem* LoadingScreen = GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

// Execute heavy work on background thread
LoadingScreen->ExecuteAsyncTaskWithLoadingScreen(
    []()
    {
        // This runs on a WORKER THREAD - no game thread hitch!
        // Do expensive computation, file I/O, etc.
        FPlatformProcess::Sleep(5.0f);
    },
    FOnCustomTaskComplete::CreateLambda([this]()
    {
        // This callback runs on GAME THREAD when task completes
        UE_LOG(LogTemp, Log, TEXT("Background task complete!"));
    })
);
// Loading screen automatically shows and hides
```

## Events

### Blueprint Events
- **On Loading Screen Show**: Fired when loading screen appears
- **On Loading Screen Hide**: Fired when loading screen disappears

Bind to these in Blueprint to run custom logic (audio, analytics, etc.)

## Advanced: Custom Widget

Create a Blueprint Widget derived from `LoadingScreenWidget`:
1. Implement `OnLoadingStart`, `OnLoadingProgress`, `OnLoadingComplete` events
2. Use `UpdateProgress` and `SetLoadingTip` to update UI
3. Configure in settings to use your custom widget

## Technical Details

### Threading Model
- Asset loading: Uses Unreal's StreamableManager (async I/O thread)
- Custom tasks: Execute on `AnyBackgroundThreadNormalTask`
- Callbacks: Always execute on game thread
- Thread-safe tracking: Uses FCriticalSection for task counting

### Automatic Loading Screen Behavior
- **PreLoadMap**: Loading screen shows before level load starts
- **PostLoadMap**: Loading screen hides 0.5s after level loads
- **Manual Override**: Set `bManualControl = true` to prevent auto-hide

### Module
- **Type**: Runtime
- **Loading Phase**: PreLoadingScreen
- **Subsystem**: GameInstanceSubsystem
- **Dependencies**: Core, Engine, Slate, SlateCore, UMG, MoviePlayer, RenderCore, PreLoadScreen

## File Structure
```
Plugins/LoadingScreen/
??? LoadingScreen.uplugin
??? README.md
??? Source/LoadingScreen/
?   ??? LoadingScreen.Build.cs
?   ??? Public/
?   ?   ??? LoadingScreenModule.h
?   ?   ??? LoadingScreenSettings.h
?   ?   ??? LoadingScreenSubsystem.h       (Main subsystem)
?   ?   ??? LoadingScreenWidget.h          (Custom widget base)
?   ?   ??? LoadingScreenAsyncTask.h       (Thread task helper)
?   ?   ??? AsyncLoadWithLoadingScreen.h   (Blueprint async node)
?   ??? Private/
?       ??? LoadingScreenModule.cpp
?       ??? LoadingScreenSettings.cpp
?       ??? LoadingScreenSubsystem.cpp
?       ??? LoadingScreenWidget.cpp
?       ??? AsyncLoadWithLoadingScreen.cpp
```

## Examples

### Example 1: Load Multiple Assets Before Starting Match
```cpp
void AMyGameMode::PrepareMatch()
{
    ULoadingScreenSubsystem* LoadingScreen = GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

    TArray<FSoftObjectPath> MatchAssets;
    MatchAssets.Add(FSoftObjectPath(TEXT("/Game/Maps/Arena.Arena")));
    MatchAssets.Add(FSoftObjectPath(TEXT("/Game/Characters/Hero.Hero")));

    LoadingScreen->LoadAssetsAsync(MatchAssets, FOnAsyncLoadComplete::CreateLambda([this]()
    {
        StartMatch();
    }));
}
```

### Example 2: Process Data on Background Thread
```cpp
void AMyActor::ProcessLargeDataSet()
{
    ULoadingScreenSubsystem* LoadingScreen = GetWorld()->GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

    LoadingScreen->ExecuteAsyncTaskWithLoadingScreen(
        [this]()
        {
            // Heavy computation on worker thread
            for (int32 i = 0; i < 1000000; i++)
            {
                // Complex calculations...
            }
        },
        FOnCustomTaskComplete::CreateUObject(this, &AMyActor::OnProcessingComplete)
    );
}
```

## Notes

- Loading screens automatically appear during level transitions
- All callbacks execute on game thread (safe to call UObject methods)
- Background tasks don't block game thread
- Multiple concurrent operations are tracked and loading screen stays until all complete
- Use `bPlayUntilStopped` for manual control over loading screen lifetime
