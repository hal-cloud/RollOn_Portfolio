//=========================================================
// xinputButton.h
// XInput ゲームパッド入力管理クラス
//
// XInputGetState を毎フレームポーリングし、ボタンとトリガーの
// 「押しっぱなし（Press）」「押した瞬間（Trigger）」「離した瞬間（Release）」
// の3状態を提供する。
// 全メンバーが static のため、インスタンス化せずに使用する。
//=========================================================
#pragma once
#include "main.h"
#include <Xinput.h>
#include <array>

class XinputButton
{
public:
    //-----------------------------------------------------
    // ライフサイクル
    // Update の dwUserIndex はコントローラーのポート番号（0〜3）
    //-----------------------------------------------------
    static void Init();
    static void Uninit();
    static void Update(DWORD dwUserIndex = 0);

    //-----------------------------------------------------
    // デジタルボタン入力状態取得
    // button には XINPUT_GAMEPAD_* のビットマスクを渡す
    // Press  : 押しっぱなしで true
    // Trigger: 押した瞬間のみ true
    // Release: 離した瞬間のみ true
    //-----------------------------------------------------
    static bool GetButtonPress(WORD button);
    static bool GetButtonTrigger(WORD button);
    static bool GetButtonRelease(WORD button);

    //-----------------------------------------------------
    // LT（L2）アナログトリガー入力状態取得
    // XINPUT_GAMEPAD_TRIGGER_THRESHOLD を閾値としてデジタル判定する
    //-----------------------------------------------------
    static bool GetLeftTriggerPress();
    static bool GetLeftTriggerTrigger();
    static bool GetLeftTriggerRelease();

    //-----------------------------------------------------
    // RT（R2）アナログトリガー入力状態取得
    //-----------------------------------------------------
    static bool GetRightTriggerPress();
    static bool GetRightTriggerTrigger();
    static bool GetRightTriggerRelease();

private:
    static WORD m_oldButtonState; // 前フレームのボタン状態（Trigger/Release 判定に使用）
    static WORD m_buttonState;    // 今フレームのボタン状態

    // アナログトリガー値（0〜255）：閾値との比較でデジタル判定する
    static BYTE m_oldLeftTrigger;
    static BYTE m_leftTrigger;
    static BYTE m_oldRightTrigger;
    static BYTE m_rightTrigger;
};
