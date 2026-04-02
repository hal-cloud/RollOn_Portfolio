#pragma once
#include <cmath>
#include "judge.h"

struct JudgeWindow {
    float perfect; 
    float good;
    float miss;
};

inline JudgeType JudgeByTimeDiff(float diff, const JudgeWindow& w) {
    float ad = std::fabs(diff);
    if (ad <= w.perfect) return JudgeType::Perfect;
    if (ad <= w.good)    return JudgeType::Good;
    if (ad <= w.miss)    return JudgeType::Miss;
    return JudgeType::Miss;
}