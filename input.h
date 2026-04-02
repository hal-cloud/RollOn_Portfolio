#pragma once

class Input
{
private:
	static constexpr int kKeyCodeCount = 256;
	static constexpr int kAsyncKeyDownMask = 0x8000;
	static constexpr BYTE kKeyStatePressed = 0x80;

	static BYTE s_oldKeyState[kKeyCodeCount];
	static BYTE s_keyState[kKeyCodeCount];

public:
	static void Init();
	static void Uninit();
	static void Update();

	static bool GetKeyPress( BYTE KeyCode );
	static bool GetKeyTrigger( BYTE KeyCode );
};
