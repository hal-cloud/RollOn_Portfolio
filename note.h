#pragma once
#include "main.h"
#include "renderer.h"
#include "gameObject.h"
#include "field.h"

enum class NoteType
{
	LOW,
	MID,
	HIGH,
	CRASH
};

class BeatManager;

class Note : public GameObject
{
private:
	class Field* m_noteVisual = nullptr;
	float m_targetZ = 0.0f;
	float m_hitTime = 0.0f;
	float m_travelTime = 1.0f;
	float m_startZ = 15.0f;
	NoteType m_type = NoteType::LOW;
	bool m_judged = false;

	BeatManager* m_beatCache = nullptr;
	bool m_beatCacheResolved = false;

	BeatManager* GetBeatManager();

public:
	void Init(NoteType type);
	void Uninit() override;
	void Update() override;
	void Draw() override;

	void Reset() override;
	void Reinit(NoteType type);
	void ReturnToPool();

	float GetHitTime() const { return m_hitTime; }
	void SetHitTime(float t) { m_hitTime = t; }

	NoteType GetType() const { return m_type; }

	void MarkJudged() { m_judged = true; }
	bool IsJudged() const { return m_judged; }

	void  SetTargetZ(float z) { m_targetZ = z; }
	float GetTargetZ() const { return m_targetZ; }

	void SetTravelTime(float travelSec) { m_travelTime = travelSec; }

	void SetColor(const Vector3& color, float gamma = 1.0f);

	Vector3 GetNotePosition() const
	{
		return m_noteVisual ? m_noteVisual->GetPosition() : Vector3(0, 0, 0);
	}

	void SetNotePosition(const Vector3& pos)
	{
		if (m_noteVisual) m_noteVisual->SetPosition(pos);
	}
};