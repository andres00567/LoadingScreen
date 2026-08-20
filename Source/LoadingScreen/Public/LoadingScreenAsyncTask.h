#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

DECLARE_DELEGATE(FOnAsyncTaskComplete);

class FLoadingScreenAsyncTask : public FNonAbandonableTask
{
    friend class FAutoDeleteAsyncTask<FLoadingScreenAsyncTask>;

public:
    FLoadingScreenAsyncTask(TFunction<void()> InTaskFunction, FOnAsyncTaskComplete InOnComplete)
        : TaskFunction(MoveTemp(InTaskFunction))
        , OnComplete(InOnComplete)
    {
    }

    void DoWork()
    {
        if (TaskFunction)
        {
            TaskFunction();
        }
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadingScreenAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    void OnCompleted()
    {
        OnComplete.ExecuteIfBound();
    }

private:
    TFunction<void()> TaskFunction;
    FOnAsyncTaskComplete OnComplete;
};
