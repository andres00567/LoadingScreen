#include "LoadingScreenWidget.h"

void ULoadingScreenWidget::UpdateProgress(float NewProgress)
{
    CurrentProgress = FMath::Clamp(NewProgress, 0.0f, 1.0f);
    OnLoadingProgress(CurrentProgress);
}

void ULoadingScreenWidget::SetLoadingTip(const FText& TipText)
{
    CurrentTip = TipText;
}
