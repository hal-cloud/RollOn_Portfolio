//=========================================================
// input.cpp
// キーボード入力管理クラス実装
//=========================================================
#include "main.h"
#include "input.h"
#include "game.h"


//=========================================================
// 静的メンバーの定義
//=========================================================
BYTE Input::s_oldKeyState[kKeyCodeCount];
BYTE Input::s_keyState[kKeyCodeCount];


//=========================================================
// ライフサイクル
//=========================================================

//-----------------------------------------------------
// キー状態バッファを全てゼロクリアする
// 未初期化のゴミ値で誤検知が起きないよう、起動時に必ず呼ぶ
//-----------------------------------------------------
void Input::Init()
{
	memset(s_oldKeyState, 0, sizeof(s_oldKeyState));
	memset(s_keyState,    0, sizeof(s_keyState));
}

void Input::Uninit()
{
}

//-----------------------------------------------------
// 毎フレームのキー状態更新
// 今フレームの状態を前フレームにコピーしてから GetAsyncKeyState を呼び直す。
// GetAsyncKeyState の最上位ビット（0x8000）が立っていれば押下中と判断する。
//-----------------------------------------------------
void Input::Update()
{
	// 今フレームの状態を前フレームとして退避（Trigger/Release 判定に使用）
	memcpy(s_oldKeyState, s_keyState, sizeof(s_keyState));

	for (int i = 0; i < kKeyCodeCount; ++i)
	{
		s_keyState[i] = (GetAsyncKeyState(i) & kAsyncKeyDownMask) ? kKeyStatePressed : 0x00;
	}
}


//=========================================================
// 入力状態取得
//=========================================================

// 押しっぱなし判定：今フレームの押下フラグを確認するだけでよい
bool Input::GetKeyPress(BYTE KeyCode)
{
	return (s_keyState[KeyCode] & kKeyStatePressed);
}

//-----------------------------------------------------
// 押した瞬間の判定
// 「今フレームが押されていて、前フレームが離れていた」場合のみ true を返す。
// 両フレームを比較することで1フレームだけ true になる。
//-----------------------------------------------------
bool Input::GetKeyTrigger(BYTE KeyCode)
{
	return ((s_keyState[KeyCode] & kKeyStatePressed) && !(s_oldKeyState[KeyCode] & kKeyStatePressed));
}