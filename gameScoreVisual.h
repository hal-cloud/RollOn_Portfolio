#pragma once
#include "gameObject.h"
#include "number.h"
#include "gameScore.h"

class GameScoreVisual : public GameObject
{
public:
    void Init(int scoreX, int scoreY,
        int comboX, int comboY,
        int digitW, int digitH,
        wchar_t* numTexture);

    void Uninit() override;
    void Update() override;
    void Draw() override;

    void EnableGauge(int x, int y, int width, int height,
        wchar_t* fillTexture,
        wchar_t* backTexture = (wchar_t*)L"assets\\texture\\white.png");

    void SetGaugePosition(int x, int y);
    void SetGaugeSize(int width, int height);

private:
    Number*      m_scoreNumber = nullptr;
    Number*      m_comboNumber = nullptr;
    class Polygon2D* m_comboUI = nullptr;
    class Polygon2D* m_scoreUI = nullptr;
	class Polygon2D* m_rankBack = nullptr;
	class Polygon2D* m_rankUI = nullptr;
    wchar_t m_lastGrade = L'\0';

    class Polygon2D* m_gaugeBack = nullptr;
    class Polygon2D* m_gaugeFill = nullptr;
    int m_gaugeWidth  = 0;
    int m_gaugeHeight = 0;
    float m_gaugeBaseX = 0.0f;
    float m_gaugeBaseY = 0.0f;

    int m_lastScore = 0;
    int m_lastCombo = 0;

    void SetScoreNumPosition(int x, int y);
    void SetComboNumPosition(int x, int y);
    void SetScoreNumScale(Vector3 scale);
    void SetComboNumScale(Vector3 scale);

    void SetScoreUIPosition(int x, int y);
    void SetComboUIPosition(int x, int y);
    void SetScoreUIScale(Vector3 scale);
    void SetComboUIScale(Vector3 scale);
};
