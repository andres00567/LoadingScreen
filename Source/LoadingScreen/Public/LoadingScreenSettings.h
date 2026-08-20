#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MoviePlayer.h"
#include "LoadingScreenSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Loading Screen Settings"))
class LOADINGSCREEN_API ULoadingScreenSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    ULoadingScreenSettings();

    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen Widget", meta = (AllowedClasses = "/Script/UMG.UserWidget"))
    FSoftClassPath LoadingScreenWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen Widget")
    bool bUseCustomWidget;

    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen", meta = (AllowedClasses = "/Script/Engine.Texture2D"))
    TArray<FSoftObjectPath> LoadingImages;

    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
    TArray<FString> LoadingTips;

    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
    float MinimumLoadingScreenDisplayTime;

    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
    bool bShowLoadingWidget;

    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
    bool bMoviesAreSkippable;

    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
    bool bAutoCompleteWhenLoadingCompletes;

    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
    bool bWaitForManualStop;

    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
    bool bWaitForLevelStreamingComplete;

    /** Consecutive readiness polls required after levels, shaders, and resource streaming all report idle. */
    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen|Readiness", meta = (ClampMin = "1", ClampMax = "10"))
    int32 PostLoadStableCheckCount;

    /** Safety ceiling for unrelated resource requests that never settle. Zero disables the ceiling. */
    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen|Readiness", meta = (ClampMin = "0.0", Units = "s"))
    float MaximumPostLoadAssetWaitTime;

    /**
     * Prevent automatic map-loading screens from covering the startup flow.
     * Automatic screens are enabled after InitialMapName has finished loading.
     */
    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen|Startup")
    bool bDelayAutomaticLoadingScreensUntilInitialMap;

    /** Map that marks the end of the startup flow (package short name). */
    UPROPERTY(Config, EditAnywhere, Category = "Loading Screen|Startup", meta = (EditCondition = "bDelayAutomaticLoadingScreensUntilInitialMap"))
    FName InitialMapName;

    UPROPERTY(Config, EditAnywhere, Category = "Movies")
    TArray<FString> StartupMovies;

    void SetupLoadingScreen(
        FLoadingScreenAttributes& LoadingScreen,
        const FString& TransitionStatsTitle = FString(),
        const TArray<FString>& TransitionStatsLines = TArray<FString>()) const;
};
