#include "gameScoreVisual.h"
#include "manager.h"
#include "scene.h"
#include "gameScore.h"
#include "polygon.h"
#include <algorithm>

namespace
{
    constexpr int   kScoreNumX      = 220;
    constexpr int   kScoreNumY      = 150;
    constexpr int   kScoreUIX       = 275;
    constexpr int   kScoreUIY       = 50;
    constexpr float kScoreNumScale  = 1.0f;
    constexpr float kScoreUIScale   = 2.5f;

    constexpr float kComboNumX      = SCREEN_WIDTH * 0.875f;
    constexpr float kComboNumY      = SCREEN_HEIGHT * 0.5f;
    constexpr float kComboUIY       = SCREEN_HEIGHT * 0.5f - 100.0f;
    constexpr float kComboNumScale  = 2.0f;
    constexpr float kComboUIScale   = 4.0f;

    constexpr int   kGaugeX         = 450;
    constexpr int   kGaugeY         = 100;
    constexpr int   kGaugeW         = 500;
    constexpr int   kGaugeH         = 60;
    constexpr float kGaugeFillScaleY= 0.8f;
    constexpr float kGaugeBackGamma = 0.7f;

    constexpr const wchar_t* kRankTexS = L"assets\\texture\\Rank_S.png";
    constexpr const wchar_t* kRankTexA = L"assets\\texture\\Rank_A.png";
    constexpr const wchar_t* kRankTexB = L"assets\\texture\\Rank_B.png";
    constexpr const wchar_t* kRankTexC = L"assets\\texture\\Rank_C.png";
}

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

    m_rankBack = Manager::GetScene()->AddGameObject<Polygon2D>(
        LAYER_UI, 110, 100, 175, 175, (wchar_t*)L"assets\\texture\\black.png");
    m_rankBack->SetGamma(0.7f);

    m_rankUI = Manager::GetScene()->AddGameObject<Polygon2D>(
        LAYER_UI, 110, 100, 175, 175, (wchar_t*)kRankTexC);
    m_lastGrade = L'C';
}

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

void GameScoreVisual::Update()
{
    const auto& cur = GameScore::Instance().GetCurrent();

    if (m_scoreNumber && cur.score != m_lastScore) {
        const int delta = cur.score - m_lastScore;
        m_scoreNumber->AddSmooth(delta, 0.1f);
        m_lastScore = cur.score;
    }

    if (m_comboNumber) m_comboNumber->SetValue(cur.combo);

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

    if (m_gaugeFill && m_gaugeWidth > 0) {
        float rate = static_cast<float>(cur.rate);
        rate = std::max(0.0f, std::min(1.0f, rate));

        Vector3 fillColor(0.2f, 1.0f, 0.4f);
        if (rate >= 0.9f)       fillColor = Vector3(1.0f, 0.9f, 0.2f);
        else if (rate >= 0.75f)  fillColor = Vector3(0.2f, 0.6f, 1.0f);
        else if (rate >= 0.6f)  fillColor = Vector3(1.0f, 0.2f, 0.2f);

        m_gaugeFill->SetColor(fillColor);

        const float centerOffsetX = (rate - 1.0f) * 0.5f * static_cast<float>(m_gaugeWidth);
        m_gaugeFill->SetScale(Vector3(rate, kGaugeFillScaleY, 1.0f));
        m_gaugeFill->SetPosition(Vector3(m_gaugeBaseX + centerOffsetX, m_gaugeBaseY, 0.0f));
    }
}

void GameScoreVisual::Draw()
{
}

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

void GameScoreVisual::EnableGauge(int x, int y, int width, int height,
    wchar_t* fillTexture,
    wchar_t* backTexture)
{
    m_gaugeWidth = width;
    m_gaugeHeight = height;
    m_gaugeBaseX = static_cast<float>(x);
    m_gaugeBaseY = static_cast<float>(y);

    m_gaugeBack = Manager::GetScene()->AddGameObject<Polygon2D>(
        LAYER_UI, x, y, width, height, backTexture);
    m_gaugeBack->SetGamma(kGaugeBackGamma);

    m_gaugeFill = Manager::GetScene()->AddGameObject<Polygon2D>(
        LAYER_UI, x, y, width, height, fillTexture);
    m_gaugeFill->SetPixelShaderFromFile("shader\\scoreFillPS.cso");
    m_gaugeFill->SetColor(Vector3(1.0f, 1.0f, 1.0f));
}

void GameScoreVisual::SetGaugePosition(int x, int y)
{
    m_gaugeBaseX = static_cast<float>(x);
    m_gaugeBaseY = static_cast<float>(y);

    if (m_gaugeBack) m_gaugeBack->SetPosition(Vector3(m_gaugeBaseX, m_gaugeBaseY, 0.0f));
    if (m_gaugeFill) m_gaugeFill->SetPosition(Vector3(m_gaugeBaseX, m_gaugeBaseY, 0.0f));
}

void GameScoreVisual::SetGaugeSize(int width, int height)
{
    m_gaugeWidth = width;
    m_gaugeHeight = height;
    if (m_gaugeBack) m_gaugeBack->SetScale(Vector3(1.0f, 1.0f, 1.0f));
    if (m_gaugeFill) m_gaugeFill->SetScale(Vector3(1.0f, 1.0f, 1.0f));
}