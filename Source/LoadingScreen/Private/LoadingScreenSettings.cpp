#include "LoadingScreenSettings.h"
#include "SLoadingScreenDefault.h"
#include "Engine/Texture2D.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#endif
#include "RenderingThread.h"
#include "Slate/DeferredCleanupSlateBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSafeZone.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/Layout/SBorder.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "SLoadingScreenProgress.h"

namespace
{
/**
 * Owns the deferred brush for exactly as long as Slate owns the image widget.
 * SImage stores only a raw FSlateBrush pointer, so a function-local shared
 * reference can otherwise expire while the movie-player thread is rendering.
 */
class SLoadingScreenTexture : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLoadingScreenTexture)
    {
    }
    SLATE_END_ARGS()

    void Construct(
        const FArguments& InArgs,
        UTexture2D* LoadingTexture)
    {
        Brush = FDeferredCleanupSlateBrush::CreateBrush(LoadingTexture);
        ChildSlot
        [
            SNew(SImage)
            .Image(Brush->GetSlateBrush())
        ];
    }

private:
    TSharedPtr<FDeferredCleanupSlateBrush> Brush;
};
}

ULoadingScreenSettings::ULoadingScreenSettings()
{
    MinimumLoadingScreenDisplayTime = 3.0f;
    bShowLoadingWidget = true;
    bMoviesAreSkippable = true;
    bAutoCompleteWhenLoadingCompletes = false;
    bWaitForManualStop = false;
    bWaitForLevelStreamingComplete = true;
    PostLoadStableCheckCount = 2;
    MaximumPostLoadAssetWaitTime = 10.0f;
    bDelayAutomaticLoadingScreensUntilInitialMap = true;
    InitialMapName = TEXT("MainMenu");
    bUseCustomWidget = false;

    LoadingTips = {
        TEXT("\"Hell did not open beneath us. It opened within us.\""),
        TEXT("\"The cities still burn, though no living hand tends the flame.\""),
        TEXT("\"Where the Infernalis gather, even the dead are afraid.\""),
        TEXT("\"Morning comes out of habit. It no longer brings hope.\""),
        TEXT("\"We buried our saints first. The Infernalis unearthed them.\""),
        TEXT("\"Every road leads deeper into what remains of the world.\""),
        TEXT("\"Do not mistake their screams for pain. The Infernalis rejoice.\""),
        TEXT("\"Survival is only the slowest way to enter Hell.\"")
    };
}

void ULoadingScreenSettings::SetupLoadingScreen(
    FLoadingScreenAttributes& LoadingScreen,
    const FString& TransitionStatsTitle,
    const TArray<FString>& TransitionStatsLines) const
{
    LoadingScreen.MinimumLoadingScreenDisplayTime = MinimumLoadingScreenDisplayTime;
    LoadingScreen.bAutoCompleteWhenLoadingCompletes = bAutoCompleteWhenLoadingCompletes;
    LoadingScreen.bMoviesAreSkippable = bMoviesAreSkippable;
    LoadingScreen.bWaitForManualStop = bWaitForManualStop;

    if (StartupMovies.Num() > 0)
    {
        LoadingScreen.MoviePaths = StartupMovies;
    }

    // Priority 1: Custom UMG Widget
    if (bUseCustomWidget && !LoadingScreenWidgetClass.IsNull())
    {
        UClass* WidgetClass = LoadingScreenWidgetClass.TryLoadClass<UUserWidget>();
        if (WidgetClass)
        {
            // For now, show a placeholder. Full UMG support requires runtime world context
            LoadingScreen.WidgetLoadingScreen = SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("BlackBrush"))
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Fill)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Custom Widget Loading...\n\n(Full UMG support coming soon)")))
                    .Justification(ETextJustify::Center)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 32))
                    .ColorAndOpacity(FLinearColor::White)
                ];
            return;
        }
    }

    // Priority 2: Custom Images + Tips
    if (bShowLoadingWidget && LoadingImages.Num() > 0)
    {
        const int32 RandomImageIndex =
            FMath::RandRange(0, LoadingImages.Num() - 1);
        const FSoftObjectPath& SelectedImagePath =
            LoadingImages[RandomImageIndex];
        UObject* ImageObject = SelectedImagePath.TryLoad();

        if (UTexture2D* LoadingTexture = Cast<UTexture2D>(ImageObject))
        {
            // Movie-player Slate can render on its own thread while the game
            // thread is blocked loading a map. Ensure the selected texture has
            // a resident render resource before handing its brush to Slate.
#if WITH_EDITOR
            FTextureCompilingManager::Get().FinishCompilation(
                {LoadingTexture});
#endif
            LoadingTexture->UpdateResource();
            LoadingTexture->SetForceMipLevelsToBeResident(30.0f);
            LoadingTexture->WaitForStreaming();
            FlushRenderingCommands();

            TSharedRef<SVerticalBox> Foreground = SNew(SVerticalBox);

            Foreground->AddSlot().FillHeight(0.25f);
            if (!TransitionStatsLines.IsEmpty())
            {
                Foreground->AddSlot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(FMargin(24.0f, 8.0f))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(
                        TransitionStatsTitle.IsEmpty()
                            ? TEXT("SEGMENT COMPLETE")
                            : TransitionStatsTitle))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 42))
                    .ColorAndOpacity(FLinearColor(0.95f, 0.72f, 0.16f, 1.0f))
                    .ShadowOffset(FVector2D(2.0f, 2.0f))
                    .ShadowColorAndOpacity(FLinearColor::Black)
                ];
                const TArray<FString> StatHeaders = {
                    TEXT("SURVIVOR"), TEXT("KILLS"), TEXT("SPECIAL"),
                    TEXT("TANK DMG"), TEXT("REVIVES"), TEXT("FRIENDLY FIRE")
                };
                const TArray<float> StatColumnWidths = {
                    180.0f, 105.0f, 145.0f, 145.0f, 125.0f, 190.0f
                };
                for (int32 LineIndex = 0;
                     LineIndex < TransitionStatsLines.Num(); ++LineIndex)
                {
                    TArray<FString> Cells;
                    if (LineIndex == 0)
                    {
                        Cells = StatHeaders;
                    }
                    else
                    {
                        TransitionStatsLines[LineIndex].ParseIntoArrayWS(Cells);
                    }

                    TSharedRef<SHorizontalBox> StatRow = SNew(SHorizontalBox);
                    for (int32 ColumnIndex = 0;
                         ColumnIndex < StatColumnWidths.Num(); ++ColumnIndex)
                    {
                        const FString CellText = Cells.IsValidIndex(ColumnIndex)
                            ? Cells[ColumnIndex] : FString();
                        StatRow->AddSlot()
                        .AutoWidth()
                        [
                            SNew(SBox)
                            .WidthOverride(StatColumnWidths[ColumnIndex])
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(CellText))
                                .Font(FCoreStyle::GetDefaultFontStyle(
                                    LineIndex == 0 ? "Bold" : "Regular", 26))
                                .ColorAndOpacity(FLinearColor::White)
                                .ShadowOffset(FVector2D(2.0f, 2.0f))
                                .ShadowColorAndOpacity(FLinearColor::Black)
                                .Justification(ETextJustify::Center)
                            ]
                        ];
                    }

                    Foreground->AddSlot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    .Padding(FMargin(24.0f, 3.0f))
                    [
                        StatRow
                    ];
                }
            }
            Foreground->AddSlot().FillHeight(0.75f);
            if (LoadingTips.Num() > 0)
            {
                const int32 RandomTipIndex =
                    FMath::RandRange(0, LoadingTips.Num() - 1);
                Foreground->AddSlot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(FMargin(32.0f, 12.0f, 32.0f, 170.0f))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(LoadingTips[RandomTipIndex]))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 30))
                    .ColorAndOpacity(FLinearColor::White)
                    .ShadowOffset(FVector2D(2.0f, 2.0f))
                    .ShadowColorAndOpacity(FLinearColor::Black)
                    .Justification(ETextJustify::Center)
                    .AutoWrapText(true)
                ];
            }
            TSharedRef<SOverlay> ScreenComposition =
                SNew(SOverlay)
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Fill)
                [
                    SNew(SScaleBox)
                    .Stretch(EStretch::ScaleToFill)
                    [
                        SNew(SLoadingScreenTexture, LoadingTexture)
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Fill)
                [
                    SNew(SSafeZone)
                    .HAlign(HAlign_Fill)
                    .VAlign(VAlign_Fill)
                    [
                        Foreground
                    ]
                ]
                // Last slot paints above both artwork and results content.
                + SOverlay::Slot()
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Bottom)
                .Padding(FMargin(48.0f, 0.0f, 48.0f, 48.0f))
                [
                    SNew(SBox)
                    .WidthOverride(1500.0f)
                    [
                        SNew(SLoadingScreenProgress)
                    ]
                ];

            LoadingScreen.WidgetLoadingScreen =
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("BlackBrush"))
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Fill)
                [
                    SNew(SScaleBox)
                    .Stretch(EStretch::ScaleToFit)
                    .StretchDirection(EStretchDirection::Both)
                    [
                        SNew(SBox)
                        .WidthOverride(1920.0f)
                        .HeightOverride(1080.0f)
                        [
                            ScreenComposition
                        ]
                    ]
                ];
            UE_LOG(LogTemp, Display,
                TEXT("LoadingScreen: selected background=%s size=%dx%d resource=%s"),
                *SelectedImagePath.ToString(),
                FMath::RoundToInt(LoadingTexture->GetSurfaceWidth()),
                FMath::RoundToInt(LoadingTexture->GetSurfaceHeight()),
                LoadingTexture->GetResource() ? TEXT("ready") : TEXT("missing"));
            return;
        }

        UE_LOG(LogTemp, Error,
            TEXT("LoadingScreen: failed to load configured background=%s object=%s; using default widget"),
            *SelectedImagePath.ToString(), *GetNameSafe(ImageObject));
    }

    // Priority 3: Default Animated "LOADING ?" Widget (Always available as fallback)
    LoadingScreen.WidgetLoadingScreen = SNew(SLoadingScreenDefault);
}
