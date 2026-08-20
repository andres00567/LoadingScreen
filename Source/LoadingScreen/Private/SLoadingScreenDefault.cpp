#include "SLoadingScreenDefault.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSafeZone.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "SLoadingScreenProgress.h"

void SLoadingScreenDefault::Construct(const FArguments& InArgs)
{
    AnimationTime = 0.0f;
    AnimationFrame = 0;

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("BlackBrush"))
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SNew(SSafeZone)
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .Padding(24.0f, 24.0f, 24.0f, 150.0f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("HELL RUN")))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 42))
                        .ColorAndOpacity(FLinearColor(0.82f, 0.035f, 0.02f, 1.0f))
                        .ShadowOffset(FVector2D(2.0f, 2.0f))
                        .ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .HAlign(HAlign_Center)
                    .Padding(0.0f, 7.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .Text(this, &SLoadingScreenDefault::GetAnimatedLoadingText)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
                        .ColorAndOpacity(FLinearColor(0.55f, 0.55f, 0.55f, 1.0f))
                        .Justification(ETextJustify::Center)
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Bottom)
                .Padding(24.0f, 24.0f, 24.0f, 58.0f)
                [
                    SNew(SBox)
                    .WidthOverride(760.0f)
                    [
                        SNew(SLoadingScreenProgress)
                    ]
                ]
            ]
        ]
    ];
}

void SLoadingScreenDefault::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

    AnimationTime += InDeltaTime;

    if (AnimationTime >= 0.4f)
    {
        AnimationTime = 0.0f;
        AnimationFrame = (AnimationFrame + 1) % 4;
    }
}

FText SLoadingScreenDefault::GetAnimatedLoadingText() const
{
    static const TCHAR* Frames[] =
    {
        TEXT("LOADING"),
        TEXT("LOADING ."),
        TEXT("LOADING . ."),
        TEXT("LOADING . . .")
    };
    return FText::FromString(Frames[FMath::Clamp(AnimationFrame, 0, 3)]);
}
