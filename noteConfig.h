#pragma once

namespace NoteConfig
{
    constexpr float kStartZ = 15.0f;
    constexpr float kTargetZ = 0.0f;
    constexpr float kTravelSec = 0.5f;
    constexpr float kSpeed = (kStartZ - kTargetZ) / kTravelSec;
}