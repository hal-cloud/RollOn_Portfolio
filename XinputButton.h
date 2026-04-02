#pragma once

#include "main.h"
#include <Xinput.h>
#include <array>

class XinputButton
{
public:
    static void Init();
    static void Uninit();
    static void Update(DWORD dwUserIndex = 0);

    // ボタン状態取得
    static bool GetButtonPress(WORD button);
    static bool GetButtonTrigger(WORD button);
    static bool GetButtonRelease(WORD button);

    // L2/LT
    static bool GetLeftTriggerPress();
    static bool GetLeftTriggerTrigger();
    static bool GetLeftTriggerRelease();

    // R2/RT
    static bool GetRightTriggerPress();
    static bool GetRightTriggerTrigger();
    static bool GetRightTriggerRelease();

private:
    static WORD m_oldButtonState;
    static WORD m_buttonState;

    // トリガー値（0～255）を保存
    static BYTE m_oldLeftTrigger;
    static BYTE m_leftTrigger;
    static BYTE m_oldRightTrigger;
    static BYTE m_rightTrigger;
};
