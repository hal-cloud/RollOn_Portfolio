#pragma once
#include "main.h"
#include "manager.h"
#include "gameObject.h"
#include <unordered_map>

static constexpr int kMaxLane = 4;

enum class LaneType
{
	LOW,
	MID,
	HIGH,
	CRASH,
	DEFAULT
};

class Lane : public GameObject
{
private:
	std::unordered_map<LaneType, class Field*> m_lanes;
	float m_laneWidth;
public:
	void Init();
	void Uninit() override;
	void Update() override;
	void Draw() override;

	Field* GetLane(LaneType type) const 
	{ 
		auto it = m_lanes.find(type);
		if (it != m_lanes.end()) {
			return it->second;
		}
		return nullptr;
	}
	float GetLaneWidth() const { return m_laneWidth; }


	const std::unordered_map<LaneType, class Field*>& GetAllLanes() const { return m_lanes; }
};