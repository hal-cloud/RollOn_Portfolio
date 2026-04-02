#include "main.h"
#include "input.h"
#include "game.h"

BYTE Input::s_oldKeyState[kKeyCodeCount];
BYTE Input::s_keyState[kKeyCodeCount];

void Input::Init()
{
	memset(s_oldKeyState, 0, sizeof(s_oldKeyState));
	memset(s_keyState, 0, sizeof(s_keyState));
}

void Input::Uninit()
{
}

void Input::Update()
{
	memcpy(s_oldKeyState, s_keyState, sizeof(s_keyState));

	for (int i = 0; i < kKeyCodeCount; ++i) {
		s_keyState[i] = (GetAsyncKeyState(i) & kAsyncKeyDownMask) ? kKeyStatePressed : 0x00;
	}
}

bool Input::GetKeyPress(BYTE KeyCode)
{
	return (s_keyState[KeyCode] & kKeyStatePressed);
}

bool Input::GetKeyTrigger(BYTE KeyCode)
{
	return ((s_keyState[KeyCode] & kKeyStatePressed) && !(s_oldKeyState[KeyCode] & kKeyStatePressed));
}