#pragma once

#include "atlas.h"

class Number : public Atlas
{
public:
    enum class Align { Center, Left, Right };

    void SetValue(int value);
    void TweenTo(int target, float durationSec);
    void AddSmooth(int delta, float durationSec);
    bool IsTweening() const { return m_isTweening; }

    void SetAlign(Align a) { m_align = a; }

    void Update() override;
    void Draw() override;

private:
    int   m_value = 0;
    int   m_targetValue = 0;
    int   m_startValue = 0;
    double m_displayValue = 0.0;

    float m_tweenDuration = 0.0f;
    float m_tweenElapsed  = 0.0f;
    bool  m_isTweening    = false;

    Align m_align = Align::Center;
};
