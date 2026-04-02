#pragma once

namespace GameSettings
{
    struct Data
    {
        float noteSpeedMultiplier = 0.5f;
    };

    inline Data& Access()
    {
        static Data s;
        return s;
    }

    inline void SetNoteSpeed(float speed)
    {
        Access().noteSpeedMultiplier = speed;
    }

    inline float GetNoteSpeed()
    {
        return Access().noteSpeedMultiplier;
    }
}