//=========================================================
// number.h
// 数値スプライト描画クラス
//
// Atlas を継承し、テクスチャアトラス（5×5 配置）から
// 0〜9 の数字を桁ごとに切り出して描画する。
// TweenTo / AddSmooth でイージング付きの数値アニメーションに対応する。
//=========================================================
#pragma once
#include "atlas.h"

class Number : public Atlas
{
public:
    //-----------------------------------------------------
    // アライメント設定
    // 複数桁の数字を描画するときの基準点を指定する
    //-----------------------------------------------------
    enum class Align
    {
        Center, // 数字全体の中心を基準点にする（デフォルト）
        Left,   // 最上位桁の左端を基準点にする
        Right,  // 最下位桁の右端を基準点にする
    };

    //-----------------------------------------------------
    // 値の設定
    // SetValue  : 即時反映（トゥイーンなし）
    // TweenTo   : 現在値から target へ durationSec 秒かけてイージングする
    // AddSmooth : 現在のターゲット値に delta を加算してトゥイーン開始する
    //             連続して呼んでも目標値が加算されていくため、連打に対応できる
    //-----------------------------------------------------
    void SetValue(int value);
    void TweenTo(int target, float durationSec);
    void AddSmooth(int delta, float durationSec);

    // トゥイーン中かどうかを返す（演出完了待ちなどに使用）
    bool IsTweening() const { return m_isTweening; }

    void SetAlign(Align a) { m_align = a; }

    void Update() override;
    void Draw() override;

private:
    int    m_value        = 0;   // 現在の表示値（描画に使用）
    int    m_targetValue  = 0;   // トゥイーンの目標値
    int    m_startValue   = 0;   // トゥイーン開始時の値
    double m_displayValue = 0.0; // 補間中の実数値（丸める前の精度を保持）

    float m_tweenDuration = 0.0f; // トゥイーン全体の秒数
    float m_tweenElapsed  = 0.0f; // トゥイーン開始からの経過秒数
    bool  m_isTweening    = false;

    Align m_align = Align::Center;
};
