//=========================================================
// gameScoreVisual.cpp
// ゲームスコア表示クラス 実装
//
// Update() で GameScore::GetCurrent() を参照し、
// スコア数字・コンボ・達成率ゲージ・ランクアイコンを毎フレーム同期する。
// ロジックと表示を分離するため、スコア計算は一切行わない。
//=========================================================
#include "gameScoreVisual.h"
#include "manager.h"
#include "scene.h"
#include "gameScore.h"
#include "polygon.h"
#include <algorithm>

namespace
{
    // スコア数字・UIラベルの画面座標
    constexpr int   kScoreNumX     = 220;
    constexpr int   kScoreNumY     = 150;
    constexpr int   kScoreUIX      = 275;
    constexpr int   kScoreUIY      = 50;
    constexpr float kScoreNumScale = 1.0f;
    constexpr float kScoreUIScale  = 2.5f;

    // コンボ数字・UIラベルの画面座標（画面右寄り中段に配置）
    constexpr float kComboNumX     = SCREEN_WIDTH * 0.875f;
    constexpr float kComboNumY     = SCREEN_HEIGHT * 0.5f;
    constexpr float kComboUIY      = SCREEN_HEIGHT * 0.5f - 100.0f; // 数字より上に配置
    constexpr float kComboNumScale = 2.0f;
    constexpr float kComboUIScale  = 4.0f;

    // 達成率ゲージの座標・サイズ
    constexpr int   kGaugeX          = 450;
    constexpr int   kGaugeY          = 100;
    constexpr int   kGaugeW          = 500;
    constexpr int   kGaugeH          = 60;
    constexpr float kGaugeFillScaleY = 0.8f;  // 背景より少し細くして視認性を上げる
    constexpr float kGaugeBackGamma  = 0.7f;  // 背景の透明度

    // ランクアイコンのテクスチャパス
    constexpr const wchar_t* kRankTexS = L"assets\\texture\\Rank_S.png";
    constexpr const wchar_t* kRankTexA = L"assets\\texture\\Rank_A.png";
    constexpr const wchar_t* kRankTexB = L"assets\\texture\\Rank_B.png";
    constexpr const wchar_t* kRankTexC = L"assets\\texture\\Rank_C.png";
}

//=========================================================
// Init
// 各 UI オブジェクトを Scene に追加し、初期位置・スケールを設定する。
//
// EnableGauge / ランクアイコンも内部で生成する。
// ランクアイコンは初期値 C で生成し、Update() で変化を検出次第差し替える。
//=========================================================
void GameScoreVisual::Init(int scoreX, int scoreY,
    int comboX, int comboY,
    int digitW, int digitH,
    wchar_t* numTexture)
{
    m_scoreNumber = Manager::GetScene()->AddGameObject<Number>(LAYER_UI, scoreX, scoreY, digitW, digitH, numTexture);
    m_scoreNumber->SetAlign(Number::Align::Left);
    m_comboNumber = Manager::GetScene()->AddGameObject<Number>(LAYER_UI, comboX, comboY, digitW, digitH, numTexture);
    m_scoreUI = Manager::GetScene()->AddGameObject<Polygon2D>(LAYER_UI, scoreX, scoreY, digitW, digitW, (wchar_t*)L"assets\\texture\\Score.png");
    m_comboUI = Manager::GetScene()->AddGameObject<Polygon2D>(LAYER_UI, comboX, comboY, digitW, digitW, (wchar_t*)L"assets\\texture\\Combo.png");

    SetScoreNumPosition(kScoreNumX, kScoreNumY);
    SetScoreUIPosition(kScoreUIX, kScoreUIY);

    SetComboNumPosition(kComboNumX, kComboNumY);
    SetComboUIPosition(kComboNumX, kComboUIY);

    SetScoreNumScale(Vector3(kScoreNumScale, kScoreNumScale, 0.0f));
    SetScoreUIScale(Vector3(kScoreUIScale, kScoreUIScale, 0.0f));

    SetComboNumScale(Vector3(kComboNumScale, kComboNumScale, 0.0f));
    SetComboUIScale(Vector3(kComboUIScale, kComboUIScale, 0.0f));

    EnableGauge
    (
        kGaugeX, kGaugeY, kGaugeW, kGaugeH,
        (wchar_t*)L"assets\\texture\\white.png",
        (wchar_t*)L"assets\\texture\\black.png"
    );

    // ランクアイコン背景（半透明黒）
    m_rankBack = Manager::GetScene()->AddGameObject<Polygon2D>(
        LAYER_UI, 110, 100, 175, 175, (wchar_t*)L"assets\\texture\\black.png");
    m_rankBack->SetAlpha(0.7f);

    // ランクアイコン本体（初期値 C。Update() でグレード変化時に差し替える）
    m_rankUI = Manager::GetScene()->AddGameObject<Polygon2D>(
        LAYER_UI, 110, 100, 175, 175, (wchar_t*)kRankTexC);
    m_lastGrade = L'C';
}

//=========================================================
// Uninit
// 保持している UI オブジェクトをすべて SetDestroy() してシーンから除去する。
// ポインタのOwnershipは Scene にあるため delete は行わない。
//=========================================================
void GameScoreVisual::Uninit()
{
    if (m_scoreNumber) m_scoreNumber->SetDestroy();
    if (m_comboNumber) m_comboNumber->SetDestroy();
    if (m_scoreUI)     m_scoreUI->SetDestroy();
    if (m_comboUI)     m_comboUI->SetDestroy();
    if (m_gaugeBack)   m_gaugeBack->SetDestroy();
    if (m_gaugeFill)   m_gaugeFill->SetDestroy();
    if (m_rankBack)    m_rankBack->SetDestroy();
    if (m_rankUI)      m_rankUI->SetDestroy();
}

//=========================================================
// Update
// GameScore::GetCurrent() から最新値を取得して UI を更新する。
//
// スコア : AddSmooth() でアニメーションしながら目標値へ近づける
// コンボ : 毎フレーム SetValue() で即時反映する
// ランク : m_lastGrade と比較して変化があった時のみテクスチャを差し替える
// ゲージ : 達成率に応じて幅（X スケール）と色を更新する
//          フィルの左端を固定するため centerOffsetX でオフセットを補正する
//=========================================================
void GameScoreVisual::Update()
{
    const auto& cur = GameScore::Instance().GetCurrent();

    // スコア：差分だけ AddSmooth することで滑らかにカウントアップさせる
    if (m_scoreNumber && cur.score != m_lastScore) {
        const int delta = cur.score - m_lastScore;
        m_scoreNumber->AddSmooth(delta, 0.1f);
        m_lastScore = cur.score;
    }

    // コンボ：即時反映
    if (m_comboNumber) m_comboNumber->SetValue(cur.combo);

    // ランク：グレードが変化したときのみテクスチャを差し替える
    if (m_rankUI && cur.grade != m_lastGrade) {
        const wchar_t* tex = kRankTexC;
        switch (cur.grade) {
        case L'S': tex = kRankTexS; break;
        case L'A': tex = kRankTexA; break;
        case L'B': tex = kRankTexB; break;
        default:   tex = kRankTexC; break;
        }
        m_rankUI->SetTexture((wchar_t*)tex);
        m_lastGrade = cur.grade;
    }

    // 達成率ゲージ：rate を [0, 1] にクランプしてスケールと色を更新する
    if (m_gaugeFill && m_gaugeWidth > 0) {
        float rate = static_cast<float>(cur.rate);
        rate = std::max(0.0f, std::min(1.0f, rate));

        // 達成率に応じてゲージ色を変える（緑→青→赤→金）
        Vector3 fillColor(0.2f, 1.0f, 0.4f);
        if (rate >= 0.9f)        fillColor = Vector3(1.0f, 0.9f, 0.2f); // S 帯：金
        else if (rate >= 0.75f)  fillColor = Vector3(0.2f, 0.6f, 1.0f); // A 帯：青
        else if (rate >= 0.6f)   fillColor = Vector3(1.0f, 0.2f, 0.2f); // B 帯：赤

        m_gaugeFill->SetColor(fillColor);

        // フィルは左端固定で右方向に伸ばすため、X スケール変化分だけ左へオフセットする
        const float centerOffsetX = (rate - 1.0f) * 0.5f * static_cast<float>(m_gaugeWidth);
        m_gaugeFill->SetScale(Vector3(rate, kGaugeFillScaleY, 1.0f));
        m_gaugeFill->SetPosition(Vector3(m_gaugeBaseX + centerOffsetX, m_gaugeBaseY, 0.0f));
    }
}

void GameScoreVisual::Draw()
{
    // 描画は各 UI オブジェクトが自律的に行うため、ここでは何もしない
}

//=========================================================
// 位置・スケールのセッタ群
// nullptr チェックを挟み、オブジェクトが未生成の場合は無視する
//=========================================================
void GameScoreVisual::SetScoreNumPosition(int x, int y)
{
    if (m_scoreNumber) m_scoreNumber->SetPosition(Vector3((float)x, (float)y, 0.0f));
}
void GameScoreVisual::SetComboNumPosition(int x, int y)
{
    if (m_comboNumber) m_comboNumber->SetPosition(Vector3((float)x, (float)y, 0.0f));
}
void GameScoreVisual::SetScoreNumScale(Vector3 scale)
{
    if (m_scoreNumber) m_scoreNumber->SetScale(scale);
}
void GameScoreVisual::SetComboNumScale(Vector3 scale)
{
    if (m_comboNumber) m_comboNumber->SetScale(scale);
}

void GameScoreVisual::SetScoreUIPosition(int x, int y)
{
    if (m_scoreUI) m_scoreUI->SetPosition(Vector3((float)x, (float)y, 0.0f));
}
void GameScoreVisual::SetComboUIPosition(int x, int y)
{
    if (m_comboUI) m_comboUI->SetPosition(Vector3((float)x, (float)y, 0.0f));
}
void GameScoreVisual::SetScoreUIScale(Vector3 scale)
{
    if (m_scoreUI) m_scoreUI->SetScale(scale);
}
void GameScoreVisual::SetComboUIScale(Vector3 scale)
{
    if (m_comboUI) m_comboUI->SetScale(scale);
}

//=========================================================
// EnableGauge
// 達成率ゲージを生成して有効化する。
//
// m_gaugeBack : 半透明の背景ポリゴン
// m_gaugeFill : 達成率に応じて幅が変化するフィルポリゴン
//=========================================================
void GameScoreVisual::EnableGauge(int x, int y, int width, int height,
    wchar_t* fillTexture,
    wchar_t* backTexture)
{
    m_gaugeWidth  = width;
    m_gaugeHeight = height;
    m_gaugeBaseX  = static_cast<float>(x);
    m_gaugeBaseY  = static_cast<float>(y);

    m_gaugeBack = Manager::GetScene()->AddGameObject<Polygon2D>(
        LAYER_UI, x, y, width, height, backTexture);
    m_gaugeBack->SetAlpha(kGaugeBackGamma);

    m_gaugeFill = Manager::GetScene()->AddGameObject<Polygon2D>(
        LAYER_UI, x, y, width, height, fillTexture);
    m_gaugeFill->SetPixelShaderFromFile("shader\\scoreFillPS.cso");
    m_gaugeFill->SetColor(Vector3(1.0f, 1.0f, 1.0f));
}

//=========================================================
// SetGaugePosition
// ゲージの基準座標（左端中央）を変更する。
// back と fill の両方を同時に移動させて位置ズレを防ぐ。
//=========================================================
void GameScoreVisual::SetGaugePosition(int x, int y)
{
    m_gaugeBaseX = static_cast<float>(x);
    m_gaugeBaseY = static_cast<float>(y);

    if (m_gaugeBack) m_gaugeBack->SetPosition(Vector3(m_gaugeBaseX, m_gaugeBaseY, 0.0f));
    if (m_gaugeFill) m_gaugeFill->SetPosition(Vector3(m_gaugeBaseX, m_gaugeBaseY, 0.0f));
}

//=========================================================
// SetGaugeSize
// ゲージの最大幅・高さを変更する。
// スケールを 1.0f にリセットして Update() 側の計算に委ねる。
//=========================================================
void GameScoreVisual::SetGaugeSize(int width, int height)
{
    m_gaugeWidth  = width;
    m_gaugeHeight = height;
    if (m_gaugeBack) m_gaugeBack->SetScale(Vector3(1.0f, 1.0f, 1.0f));
    if (m_gaugeFill) m_gaugeFill->SetScale(Vector3(1.0f, 1.0f, 1.0f));
}