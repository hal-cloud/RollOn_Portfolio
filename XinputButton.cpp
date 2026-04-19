//=========================================================
// xinputButton.cpp
// XInput ゲームパッド入力管理クラス実装
//=========================================================
#include "XinputButton.h"


//=========================================================
// 静的メンバーの定義
//=========================================================
WORD XinputButton::m_oldButtonState = 0;
WORD XinputButton::m_buttonState    = 0;

BYTE XinputButton::m_oldLeftTrigger  = 0;
BYTE XinputButton::m_leftTrigger     = 0;
BYTE XinputButton::m_oldRightTrigger = 0;
BYTE XinputButton::m_rightTrigger    = 0;


//=========================================================
// ライフサイクル
//=========================================================

void XinputButton::Init()
{
}

void XinputButton::Uninit()
{
}

//-----------------------------------------------------
// 毎フレームのパッド状態更新
// 今フレームの状態を前フレームに退避してから XInputGetState を呼び直す。
// 接続されていない場合（ERROR_SUCCESS 以外）は全入力を 0 にリセットして
// 未接続のゴミ値で誤検知が起きないようにする。
//-----------------------------------------------------
void XinputButton::Update(DWORD dwUserIndex)
{
    // 今フレームの状態を前フレームとして退避（Trigger/Release 判定に使用）
    m_oldButtonState  = m_buttonState;
    m_oldLeftTrigger  = m_leftTrigger;
    m_oldRightTrigger = m_rightTrigger;

    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));
    if (XInputGetState(dwUserIndex, &state) == ERROR_SUCCESS)
    {
        m_buttonState  = state.Gamepad.wButtons;
        m_leftTrigger  = state.Gamepad.bLeftTrigger;
        m_rightTrigger = state.Gamepad.bRightTrigger;
    }
    else
    {
        // コントローラー未接続時は全入力をリセット
        m_buttonState  = 0;
        m_leftTrigger  = 0;
        m_rightTrigger = 0;
    }
}


//=========================================================
// デジタルボタン入力状態取得
//=========================================================

// 押しっぱなし：今フレームのビットが立っていれば true
bool XinputButton::GetButtonPress(WORD button)
{
    return (m_buttonState & button) != 0;
}

// 押した瞬間：今フレームが ON かつ前フレームが OFF のときのみ true
bool XinputButton::GetButtonTrigger(WORD button)
{
    return ((m_buttonState & button) != 0) && ((m_oldButtonState & button) == 0);
}

// 離した瞬間：今フレームが OFF かつ前フレームが ON のときのみ true
bool XinputButton::GetButtonRelease(WORD button)
{
    return ((m_buttonState & button) == 0) && ((m_oldButtonState & button) != 0);
}


//=========================================================
// アナログトリガー入力状態取得
// XINPUT_GAMEPAD_TRIGGER_THRESHOLD を閾値としてデジタル判定する
//=========================================================

bool XinputButton::GetLeftTriggerPress()
{
    return m_leftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
}

bool XinputButton::GetLeftTriggerTrigger()
{
    return (m_leftTrigger     > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        && (m_oldLeftTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

bool XinputButton::GetLeftTriggerRelease()
{
    return (m_leftTrigger     <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        && (m_oldLeftTrigger   > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

bool XinputButton::GetRightTriggerPress()
{
    return m_rightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
}

bool XinputButton::GetRightTriggerTrigger()
{
    return (m_rightTrigger     > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        && (m_oldRightTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

bool XinputButton::GetRightTriggerRelease()
{
    return (m_rightTrigger     <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        && (m_oldRightTrigger   > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}