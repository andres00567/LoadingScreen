# Loading Screen Plugin - Widget Configuration Guide

## Overview
The Loading Screen Plugin now supports custom UMG widgets and automatically waits for level streaming to complete before hiding the loading screen.

## New Features

### 1. Custom Widget Support
You can now use your own UMG widget for the loading screen instead of the default Slate-based screen.

### 2. Level Streaming Awareness
The plugin automatically detects when all streaming levels are fully loaded and visible before hiding the loading screen.

### 3. Configurable Settings
All settings are exposed in **Project Settings > Plugins > Loading Screen Settings**

## Configuration

### Project Settings Location
1. Open **Project Settings**
2. Navigate to **Plugins** section
3. Find **Loading Screen Settings**

### Available Settings

#### Loading Screen Widget
- **Use Custom Widget** (bool) - Enable to use a custom UMG widget
- **Loading Screen Widget Class** (Soft Class Reference) - Select your custom UMG widget class

#### Loading Screen Behavior
- **Wait For Level Streaming Complete** (bool, default: true)
  - When enabled, the loading screen stays visible until ALL streaming levels are fully loaded
  - Checks for: Level loaded, level visible, and no pending async loads
  - Prevents showing an incomplete level to players

- **Minimum Loading Screen Display Time** (float, default: 3.0)
  - Minimum seconds the loading screen will display
  - Prevents flickering on fast loads

- **Show Loading Widget** (bool, default: true)
  - Whether to show the widget/images during loading

- **Movies Are Skippable** (bool, default: true)
  - Allow players to skip movie playback

- **Auto Complete When Loading Completes** (bool, default: false)
  - Automatically hide when loading finishes (if false, must call HideLoadingScreen manually)

- **Wait For Manual Stop** (bool, default: false)
  - Keep showing until HideLoadingScreen is called

#### Visual Content
- **Loading Images** (Array of Texture2D) - Background images (random selection)
- **Loading Tips** (Array of Strings) - Tips to display (random selection)
- **Startup Movies** (Array of Strings) - Movie files to play (MP4, WebM)

## Creating a Custom Loading Screen Widget

### Step 1: Create a UMG Widget Blueprint
1. Right-click in Content Browser
2. Select **User Interface > Widget Blueprint**
3. Name it (e.g., `WBP_LoadingScreen`)

### Step 2: Design Your Widget
Your widget can include:
- **Images** - Background art, logos, character portraits
- **Progress Bars** - Visual feedback (can be animated)
- **Text Blocks** - Loading tips, level names, status messages
- **Animations** - Spinning icons, fading elements, etc.
- **Videos** - Background video loops via Media Player

### Step 3: Configure in Settings
1. Open **Project Settings > Plugins > Loading Screen Settings**
2. Check **Use Custom Widget**
3. Set **Loading Screen Widget Class** to your widget (e.g., `WBP_LoadingScreen`)

### Example Widget Setup

```
Canvas Panel (Full Screen)
??? Image (Background) - Stretch to fill
??? Vertical Box (Center)
?   ??? Text Block (Title) - "LOADING"
?   ??? Progress Bar (Animated)
?   ??? Text Block (Tips) - Rotating loading tips
??? Image (Logo) - Bottom corner
```

## Level Streaming Behavior

### How It Works

When a level loads, the subsystem:

1. **PreLoadMap Event** - Loading screen shows
2. **Level Loads** - Main level geometry loads
3. **PostLoadMap Event** - Level is loaded, but...
4. **Streaming Level Check Begins** - Plugin checks every 0.1 seconds:
   - Are all streaming levels that should be loaded actually loaded?
   - Are all streaming levels visible?
   - Are there any pending async loads?
5. **All Complete** - Loading screen hides after 0.3 second delay

### Why This Matters

Without level streaming awareness, players would see:
- ? Partially loaded levels
- ? Pop-in of streaming actors
- ? Incomplete geometry
- ? Missing textures/materials still loading

With level streaming awareness:
- ? Fully loaded and visible level
- ? All streaming actors spawned
- ? Complete geometry
- ? Professional presentation

### Disabling Level Streaming Wait

If you want the old behavior (hide immediately after PostLoadMap):
1. Open **Project Settings > Plugins > Loading Screen Settings**
2. Uncheck **Wait For Level Streaming Complete**

## Blueprint Usage

### Get Current Loading Screen Settings

```
Get Default Object (Loading Screen Settings)
  ? Get Minimum Loading Screen Display Time
  ? Get Wait For Level Streaming Complete
  ? etc.
```

### Manual Control During Level Load

```
Get Game Instance 
  ? Get Subsystem (Loading Screen Subsystem)
  ? Show Loading Screen (Play Until Stopped: true)

// Load your level
Open Level (Level Name)

// Loading screen will automatically hide when level + streaming is complete
// OR manually hide it:
  ? Hide Loading Screen
```

### React to Loading Screen Events

```
Event Begin Play
  Get Game Instance
    ? Get Subsystem (Loading Screen Subsystem)
    ? OnLoadingScreenShow ? Add Dynamic (Event OnLoadingShow)
    ? OnLoadingScreenHide ? Add Dynamic (Event OnLoadingHide)

Event OnLoadingShow
  // Pause music, show cursor, etc.

Event OnLoadingHide
  // Resume music, hide cursor, etc.
```

## C++ Usage

### Access Settings

```cpp
const ULoadingScreenSettings* Settings = GetDefault<ULoadingScreenSettings>();

if (Settings)
{
    bool bWaits = Settings->bWaitForLevelStreamingComplete;
    float MinTime = Settings->MinimumLoadingScreenDisplayTime;
    TSubclassOf<UUserWidget> WidgetClass = Settings->LoadingScreenWidgetClass.TryLoadClass<UUserWidget>();
}
```

### Custom Widget Class

```cpp
// In your widget's header
UCLASS()
class UMyLoadingScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent)
    void OnLoadingStart();

    UFUNCTION(BlueprintImplementableEvent)
    void UpdateProgress(float Progress);

    UFUNCTION(BlueprintImplementableEvent)
    void OnLoadingComplete();
};
```

## Advanced: Custom Loading Logic

### Override Level Streaming Behavior

If you need custom logic for determining when loading is complete:

```cpp
ULoadingScreenSubsystem* LoadingScreen = GetGameInstance()->GetSubsystem<ULoadingScreenSubsystem>();

// Show loading screen with manual control
LoadingScreen->ShowLoadingScreen(true, 2.0f);

// Load level
UGameplayStatics::OpenLevel(this, LevelName);

// Implement your own completion check
GetWorld()->GetTimerManager().SetTimer(CheckTimer, [this, LoadingScreen]()
{
    if (IsMyCustomLoadingComplete())
    {
        LoadingScreen->HideLoadingScreen();
        GetWorld()->GetTimerManager().ClearTimer(CheckTimer);
    }
}, 0.1f, true);
```

## Troubleshooting

### Loading Screen Hides Too Early
- Enable **Wait For Level Streaming Complete** in settings
- Increase **Minimum Loading Screen Display Time**
- Use manual control with `ShowLoadingScreen(true, ...)`

### Loading Screen Stays Visible Forever
- Disable **Wait For Manual Stop** in settings
- Check that streaming levels are set to load properly
- Verify no errors in level streaming setup
- Manually call `HideLoadingScreen()` if needed

### Custom Widget Not Showing
- Verify **Use Custom Widget** is checked
- Ensure **Loading Screen Widget Class** is set correctly
- Note: UMG widgets have limitations during PreLoadingScreen phase
  - May need to use Slate-based widgets for very early loading
  - UMG works fine for regular level transitions

### Performance Issues
- Keep widgets simple during loading screen phase
- Avoid heavy Blueprint logic in widget Tick
- Use simple animations (avoid complex materials)
- Preload widget assets to avoid loading hitches

## Best Practices

### DO:
? Use **Wait For Level Streaming Complete** for seamless experience  
? Test with slow storage to ensure loading screen shows long enough  
? Keep custom widgets performant (loading screen should be fast!)  
? Provide loading tips to keep players engaged  
? Test on target hardware (console, low-end PC)  

### DON'T:
? Make loading screen widgets too complex  
? Disable streaming wait without testing the visual result  
? Forget to test with various level sizes  
? Use uncompressed video files (large file sizes)  
? Assume loading times on dev machine match shipping builds  

## Example Configurations

### Fast-Paced Action Game
```
Minimum Loading Screen Display Time: 2.0
Wait For Level Streaming Complete: true
Auto Complete When Loading Completes: false
Wait For Manual Stop: false
```

### Story-Driven Game with Tips
```
Minimum Loading Screen Display Time: 5.0
Wait For Level Streaming Complete: true
Show Loading Widget: true
Loading Tips: [Many story tips and hints]
```

### Competitive Multiplayer
```
Minimum Loading Screen Display Time: 1.0
Wait For Level Streaming Complete: true
Auto Complete When Loading Completes: false
Movies Are Skippable: true
```

---

For more information, see:
- **README.md** - Full plugin documentation
- **QUICKSTART.md** - Quick start guide
- **USAGE_EXAMPLES.cpp** - C++ code examples
