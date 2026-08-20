#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SLoadingScreenDefault : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLoadingScreenDefault) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    FText GetAnimatedLoadingText() const;

    float AnimationTime;
    int32 AnimationFrame;
};
