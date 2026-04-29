//=========================================================
// gameScoreVisual.h
// ゲームスコア表示クラス
//
// GameScore（ロジック）から値を取得し、スコア数字・コンボ数字・
// 達成率ゲージ・ランクアイコンを UI として描画する。
// GameObject を継承して Scene に登録し、毎フレーム Update() で
// GameScore::GetCurrent() を参照して表示を同期する。
//=========================================================
#pragma once
#include "gameObject.h"
#include "number.h"

class GameScoreVisual : public GameObject
{
public:
    //-----------------------------------------------------
    // ライフサイクル
    // Init: 各 UI オブジェクトを Scene に追加し、初期位置・スケールを設定する
    //       numTexture は数字スプライトシートのパス
    //-----------------------------------------------------
    void Init(int scoreX, int scoreY,
        int comboX, int comboY,
        int digitW, int digitH,
        wchar_t* numTexture);

    void Uninit() override;
    void Update() override;
    void Draw()   override;

    //-----------------------------------------------------
    // ゲージ
    // EnableGauge   : 達成率ゲージを生成して有効化する
    //                 backTexture を省略すると白テクスチャが使われる
    // SetGaugePosition: ゲージの基準座標（左端中央）を変更する
    // SetGaugeSize    : ゲージの最大幅・高さを変更する
    //-----------------------------------------------------
    void EnableGauge(int x, int y, int width, int height,
        wchar_t* fillTexture,
        wchar_t* backTexture = (wchar_t*)L"assets\\texture\\white.png");

    void SetGaugePosition(int x, int y);
    void SetGaugeSize(int width, int height);

private:
    //-----------------------------------------------------
    // UI オブジェクト（Scene が所有。ポインタを借りるだけで delete 不可）
    // Uninit() で SetDestroy() を呼び、Scene 側に解放を委ねる
    //-----------------------------------------------------
    Number* m_scoreNumber = nullptr;
    Number* m_comboNumber = nullptr;
    class Polygon2D* m_comboUI = nullptr;
    class Polygon2D* m_scoreUI = nullptr;
    class Polygon2D* m_rankBack = nullptr; // ランクアイコンの背景（半透明黒）
    class Polygon2D* m_rankUI = nullptr; // ランクアイコン本体（S/A/B/C）

    // 前フレームのグレードと比較し、変化があったときのみテクスチャを差し替える
    wchar_t m_lastGrade = L'\0';

    //-----------------------------------------------------
    // ゲージ
    // m_gaugeBaseX/Y はフィルの位置計算の基準点（ゲージ左端中央）
    //-----------------------------------------------------
    class Polygon2D* m_gaugeBack = nullptr;
    class Polygon2D* m_gaugeFill = nullptr;
    int   m_gaugeWidth = 0;
    int   m_gaugeHeight = 0;
    float m_gaugeBaseX = 0.0f;
    float m_gaugeBaseY = 0.0f;

    //-----------------------------------------------------
    // 差分検出用キャッシュ
    // 毎フレーム SetValue するコストを避けるため、前回値と比較してから更新する
    //-----------------------------------------------------
    int m_lastScore = 0;
    int m_lastCombo = 0;

    //-----------------------------------------------------
    // プライベートヘルパ
    // Init() 内部で位置・スケールを一括設定するためのセッタ群
    //-----------------------------------------------------
    void SetScoreNumPosition(int x, int y);
    void SetComboNumPosition(int x, int y);
    void SetScoreNumScale(Vector3 scale);
    void SetComboNumScale(Vector3 scale);

    void SetScoreUIPosition(int x, int y);
    void SetComboUIPosition(int x, int y);
    void SetScoreUIScale(Vector3 scale);
    void SetComboUIScale(Vector3 scale);
};