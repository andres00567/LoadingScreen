#include "SLoadingScreenWidgetWrapper.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"

void SLoadingScreenWidgetWrapper::Construct(const FArguments& InArgs, TSubclassOf<UUserWidget> WidgetClass)
{
    if (WidgetClass)
    {
        // Create the UMG widget instance
        UUserWidget* Widget = CreateWidget<UUserWidget>(GEngine->GameViewport->GetWorld(), WidgetClass);
        if (Widget)
        {
            UserWidget = Widget;
            WidgetContent = Widget->TakeWidget();

            ChildSlot
            [
                WidgetContent.ToSharedRef()
            ];
        }
    }
}

void SLoadingScreenWidgetWrapper::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

    // Tick the UMG widget if it's valid
    if (UserWidget.IsValid())
    {
        // UMG widgets are ticked by their owner, but during loading screens we need to manually tick
        // This is typically handled by the viewport, but we're outside normal rendering
    }
}
