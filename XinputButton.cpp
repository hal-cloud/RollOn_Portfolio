#include "XinputButton.h"

WORD XinputButton::m_oldButtonState = 0;
WORD XinputButton::m_buttonState = 0;

BYTE XinputButton::m_oldLeftTrigger = 0;
BYTE XinputButton::m_leftTrigger = 0;
BYTE XinputButton::m_oldRightTrigger = 0;
BYTE XinputButton::m_rightTrigger = 0;


void XinputButton::Init() 
{

}

void XinputButton::Uninit()
{

}


void XinputButton::Update(DWORD dwUserIndex)
{
    m_oldButtonState = m_buttonState;
    m_oldLeftTrigger = m_leftTrigger;
    m_oldRightTrigger = m_rightTrigger;

    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));
    if (XInputGetState(dwUserIndex, &state) == ERROR_SUCCESS)
    {
        m_buttonState = state.Gamepad.wButtons;
        m_leftTrigger = state.Gamepad.bLeftTrigger;
        m_rightTrigger = state.Gamepad.bRightTrigger;
    }
    else
    {
        m_buttonState = 0;
        m_leftTrigger = 0;
        m_rightTrigger = 0;
    }
}


bool XinputButton::GetButtonPress(WORD button)
{
    return (m_buttonState & button) != 0;
}
bool XinputButton::GetButtonTrigger(WORD button)
{
    return ((m_buttonState & button) != 0) && ((m_oldButtonState & button) == 0);
}
bool XinputButton::GetButtonRelease(WORD button)
{
    return ((m_buttonState & button) == 0) && ((m_oldButtonState & button) != 0);
}


bool XinputButton::GetLeftTriggerPress() {
    return m_leftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
}
bool XinputButton::GetLeftTriggerTrigger() {
    return (m_leftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) && (m_oldLeftTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}
bool XinputButton::GetLeftTriggerRelease() {
    return (m_leftTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD) && (m_oldLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}


bool XinputButton::GetRightTriggerPress() {
    return m_rightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
}
bool XinputButton::GetRightTriggerTrigger() {
    return (m_rightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) && (m_oldRightTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}
bool XinputButton::GetRightTriggerRelease() {
    return (m_rightTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD) && (m_oldRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}