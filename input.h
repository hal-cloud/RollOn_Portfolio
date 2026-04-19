//=========================================================
// input.h
// キーボード入力管理クラス
//
// Win32 の GetAsyncKeyState を毎フレームポーリングし、
// 「押しっぱなし（Press）」と「押した瞬間（Trigger）」の2状態を提供する。
// 全メンバーが static のため、インスタンス化せずに使用する。
//=========================================================
#pragma once

class Input
{
public:
	//-----------------------------------------------------
	// ライフサイクル
	//-----------------------------------------------------
	static void Init();
	static void Uninit();
	// 毎フレーム呼び出し：今フレームのキー状態を取得して前フレームと入れ替える
	static void Update();

	//-----------------------------------------------------
	// 入力状態取得
	// GetKeyPress  : 押しっぱなしで true を返す
	// GetKeyTrigger: 押した瞬間（前フレームが離れていて今フレームが押されている）のみ true
	//-----------------------------------------------------
	static bool GetKeyPress(BYTE KeyCode);
	static bool GetKeyTrigger(BYTE KeyCode);

private:
	static constexpr int  kKeyCodeCount     = 256;    // Windows 仮想キーコードの総数
	static constexpr int  kAsyncKeyDownMask = 0x8000; // GetAsyncKeyState の最上位ビットが押下を示す
	static constexpr BYTE kKeyStatePressed  = 0x80;   // 内部で「押されている」を表すフラグ値

	static BYTE s_oldKeyState[kKeyCodeCount]; // 前フレームのキー状態（Trigger 判定に使用）
	static BYTE s_keyState[kKeyCodeCount];    // 今フレームのキー状態
};
