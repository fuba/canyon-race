#pragma once
#include "CoreMinimal.h"

// Only touched on the game thread. A pending OpenXR event is not yet an applied origin change.
struct FCanyonReferenceChanges
{
    TArray<int64> PendingTimes;
    uint64 Revision=0;
    void Queue(int64 Time) { PendingTimes.AddUnique(Time); }
    bool Advance(int64 DisplayTime)
    {
        const int32 Count=PendingTimes.RemoveAll([DisplayTime](int64 Time){return Time<=DisplayTime;});
        if (Count>0) {++Revision;return true;}
        return false;
    }
};
uint64 CanyonAppliedReferenceRevision();
