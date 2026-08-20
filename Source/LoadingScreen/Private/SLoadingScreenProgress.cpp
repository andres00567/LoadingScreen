#include "SLoadingScreenProgress.h"

#include "HAL/CriticalSection.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeLock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    FCriticalSection GLoadingProgressLock;
    FString GLoadingStatus = TEXT("Preparing...");
    FString GLoadingDetails;
    float GLoadingProgress = 0.0f;
    bool GLoadingProgressIsDeterminate = false;
}

void FLoadingScreenProgressState::BeginMapLoad(const FString& MapName)
{
    const FString ShortName = FPackageName::GetShortName(MapName);
    SetIndeterminatePhase(
        FString::Printf(TEXT("Loading %s..."), *ShortName));
    SetDetails(TEXT("Waiting for the engine package loader"));
}

void FLoadingScreenProgressState::SetPhase(const FString& Status, float Progress)
{
    FScopeLock Lock(&GLoadingProgressLock);
    GLoadingStatus = Status;
    GLoadingProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
    GLoadingProgressIsDeterminate = true;
}

void FLoadingScreenProgressState::SetIndeterminatePhase(
    const FString& Status)
{
    FScopeLock Lock(&GLoadingProgressLock);
    GLoadingStatus = Status;
    GLoadingProgressIsDeterminate = false;
}

void FLoadingScreenProgressState::SetDetails(const FString& Details)
{
    FScopeLock Lock(&GLoadingProgressLock);
    GLoadingDetails = Details;
}

void FLoadingScreenProgressState::Complete()
{
    SetPhase(TEXT("Ready"), 1.0f);
}

FText FLoadingScreenProgressState::GetStatusText()
{
    FScopeLock Lock(&GLoadingProgressLock);
    return FText::FromString(GLoadingStatus);
}

FText FLoadingScreenProgressState::GetDetailsText()
{
    FScopeLock Lock(&GLoadingProgressLock);
    return FText::FromString(GLoadingDetails);
}

TOptional<float> FLoadingScreenProgressState::GetProgress()
{
    FScopeLock Lock(&GLoadingProgressLock);
    return GLoadingProgressIsDeterminate
        ? TOptional<float>(GLoadingProgress)
        : TOptional<float>();
}

void SLoadingScreenProgress::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.30f))
        .Padding(FMargin(30.0f, 20.0f, 30.0f, 22.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 11.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(this, &SLoadingScreenProgress::GetStatusText)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
                    .ColorAndOpacity(FLinearColor(0.88f, 0.88f, 0.88f, 1.0f))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(20.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(this, &SLoadingScreenProgress::GetProgressText)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
                    .ColorAndOpacity(FLinearColor(0.82f, 0.035f, 0.02f, 1.0f))
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 9.0f)
            [
                SNew(STextBlock)
                .Text(this, &SLoadingScreenProgress::GetDetailsText)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 15))
                .ColorAndOpacity(FLinearColor(0.64f, 0.64f, 0.64f, 1.0f))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBox)
                .HeightOverride(8.0f)
                [
                    SNew(SProgressBar)
                    .Percent(this, &SLoadingScreenProgress::GetProgress)
                    .FillColorAndOpacity(FLinearColor(0.9f, 0.025f, 0.01f, 1.0f))
                ]
            ]
        ]
    ];
}

FText SLoadingScreenProgress::GetStatusText() const
{
    return FLoadingScreenProgressState::GetStatusText();
}

FText SLoadingScreenProgress::GetDetailsText() const
{
    return FLoadingScreenProgressState::GetDetailsText();
}

FText SLoadingScreenProgress::GetProgressText() const
{
    const TOptional<float> Progress = FLoadingScreenProgressState::GetProgress();
    if (!Progress.IsSet())
    {
        return FText::GetEmpty();
    }
    const int32 Percentage = FMath::RoundToInt(
        FMath::Clamp(Progress.Get(0.0f), 0.0f, 1.0f) * 100.0f);
    return FText::FromString(FString::Printf(TEXT("%d%%"), Percentage));
}

TOptional<float> SLoadingScreenProgress::GetProgress() const
{
    return FLoadingScreenProgressState::GetProgress();
}
