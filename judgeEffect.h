//=========================================================
// judgeEffect.h
// 判定エフェクトクラス
//
// Perfect / Good / Miss の判定結果に対応した画像を画面中央に表示。
// JudgeSystem::SpawnJudgeEffect() から生成され、寿命が尽きると
// 自身で SetDestroy() を呼んでシーンから除去する。
//=========================================================
#pragma once

#include "main.h"
#include "renderer.h"
#include "gameObject.h"

class JudgeEffect : public GameObject
{
private:
    //-----------------------------------------------------
    // UI オブジェクト（Scene が所有。ポインタを借りるだけで delete 不可）
    // Uninit() で SetDestroy() を呼び、Scene 側に解放を委ねる
    //-----------------------------------------------------
    class Polygon2D* m_laneEffect = nullptr;

    // 寿命カウンタ。Update() で毎フレーム減算し、0 以下で自身を破棄する
    // アルファ値は lifeTime / 10.0f で計算するため、残り 10 フレームで完全透明になる
    int m_lifeTime = 60;

public:
    //-----------------------------------------------------
    // ライフサイクル
    // Init  : 判定結果画像を Polygon2D として Scene に追加する
    //         fileName で Perfect / Good / Miss のテクスチャを切り替える
    // Update: 毎フレーム上方向に移動しながらフェードアウトさせる
    //-----------------------------------------------------
    void Init(float posx, float posy, float width, float height, wchar_t* fileName);
    void Uninit() override;
    void Update() override;
};
