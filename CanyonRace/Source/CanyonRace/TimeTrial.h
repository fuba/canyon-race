#pragma once
#include "CoreMinimal.h"

struct FCanyonTimeTrial
{
    TArray<double> Best,Reference,Current;
    bool bNewBest=false;
    void StartRun() {Reference=Best;Current.Reset();bNewBest=false;}
    static bool Valid(const TArray<double>& Times,int32 Count)
    {
        if (Count<1 || Times.Num()!=Count) return false;
        double Previous=0.;
        for (double Time:Times)
        {
            if (!FMath::IsFinite(Time) || Time<=Previous || Time>86400.) return false;
            Previous=Time;
        }
        return true;
    }
    bool Gate(int32 Index,int32 Count,double Time)
    {
        if (Index!=Current.Num()+1 || Index>Count || !FMath::IsFinite(Time) || Time<=0. || (!Current.IsEmpty() && Time<=Current.Last())) return false;
        Current.Add(Time);
        if (!Valid(Current,Count) || (Valid(Best,Count) && Time>=Best.Last())) return false;
        Best=Current;bNewBest=true;return true;
    }
    bool HasDelta() const {return !Current.IsEmpty() && Reference.IsValidIndex(Current.Num()-1);}
    double Delta() const {return HasDelta()?Current.Last()-Reference[Current.Num()-1]:0.;}
    FString Serialize() const
    {
        FString Text;for (double Time:Best) Text+=FString::Printf(TEXT("%.9f\n"),Time);return Text;
    }
    bool Load(const FString& Text,int32 Count)
    {
        TArray<FString> Lines;Text.ParseIntoArrayLines(Lines);
        TArray<double> Times;
        for (const FString& Line:Lines) {if (!Line.IsNumeric()) return false;Times.Add(FCString::Atod(*Line));}
        if (!Valid(Times,Count)) return false;
        Best=MoveTemp(Times);return true;
    }
};
