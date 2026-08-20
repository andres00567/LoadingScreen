#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingScreenWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class LOADINGSCREEN_API ULoadingScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "Loading Screen")
    void OnLoadingStart();

    UFUNCTION(BlueprintImplementableEvent, Category = "Loading Screen")
    void OnLoadingProgress(float Progress);

    UFUNCTION(BlueprintImplementableEvent, Category = "Loading Screen")
    void OnLoadingComplete();

    UFUNCTION(BlueprintCallable, Category = "Loading Screen")
    void UpdateProgress(float NewProgress);

    UFUNCTION(BlueprintCallable, Category = "Loading Screen")
    void SetLoadingTip(const FText& TipText);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Loading Screen")
    float CurrentProgress;

    UPROPERTY(BlueprintReadOnly, Category = "Loading Screen")
    FText CurrentTip;
};
