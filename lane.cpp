#include "manager.h"
#include "scene.h"
#include "lane.h"
#include "field.h"

void Lane::Init()
{
    m_lanes.clear();

    constexpr LaneType laneOrder[kMaxLane] = {
        LaneType::LOW, LaneType::MID, LaneType::HIGH, LaneType::CRASH
    };

    const Vector3 laneColors[kMaxLane] = {
        Vector3(1.0f, 0.25f, 0.0f),
        Vector3(1.0f, 0.5f, 0.5f),
        Vector3(1.0f, 0.75f, 0.0f),
        Vector3(1.0f, 1.0f, 0.0f)
    };

    const float laneWidth = 2.0f;
    const float laneHeight = 30.0f;
    const float startX = -0.5f * laneWidth * (kMaxLane - 1);
    const char* laneBloomShader = "shader\\laneBloomPS.cso";

    for (int i = 0; i < kMaxLane; ++i)
    {
        Field* laneField = Manager::GetScene()->AddGameObject<Field>(
            LAYER_WORLD,
            Vector3(laneWidth * i + startX, 0.0f, 0.0f),
            laneWidth,
            laneHeight,
            (wchar_t*)L"assets\\texture\\black.png");

        laneField->SetPixelShader(laneBloomShader);
        laneField->SetGamma(0.5f);
        laneField->SetColor(laneColors[i]);

        m_lanes[laneOrder[i]] = laneField;

    }
    m_laneWidth = laneWidth;
}

void Lane::Uninit()
{
	m_lanes.clear();
}

void Lane::Update()
{
}

void Lane::Draw()
{
}
