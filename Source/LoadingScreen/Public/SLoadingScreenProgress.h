#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/** Thread-safe state shared by the game thread and MoviePlayer loading thread. */
class LOADINGSCREEN_API FLoadingScreenProgressState
{
public:
    static void BeginMapLoad(const FString& MapName);
    static void SetPhase(const FString& Status, float Progress);
    static void SetIndeterminatePhase(const FString& Status);
    static void SetDetails(const FString& Details);
    static void Complete();
    static FText GetStatusText();
    static FText GetDetailsText();
    static TOptional<float> GetProgress();
};

/** Bottom-aligned loading status and progress bar used by every screen style. */
class LOADINGSCREEN_API SLoadingScreenProgress : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLoadingScreenProgress) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FText GetStatusText() const;
    FText GetDetailsText() const;
    FText GetProgressText() const;
    TOptional<float> GetProgress() const;
};
