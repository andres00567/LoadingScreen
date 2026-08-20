#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Blueprint/UserWidget.h"

class SLoadingScreenWidgetWrapper : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLoadingScreenWidgetWrapper) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, TSubclassOf<UUserWidget> WidgetClass);

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    TSharedPtr<SWidget> WidgetContent;
    TWeakObjectPtr<UUserWidget> UserWidget;
};
