//=========================================================
// QuantizedSePlayer BGMのグリッドに合わせてSEを鳴らす簡易スケジューラクラス
// オートプレイ時のSE再生をBGMのグリッドに合わせて行うため作成
//=========================================================
#pragma once
#include <string>
#include <cstdint>
#include <cmath>
#include "soundManager.h"

//-------------------------------------
// グリッド分割の種類
//-------------------------------------
enum class GridDivision : int
{
    Sixteenth = 4,   // 16分音符
    ThirtySecond = 8 // 32分音符
};

// 入力を「次のグリッド」で再生する簡易スケジューラ
class QuantizedSePlayer
{
public:
    QuantizedSePlayer(const std::string& bgmKey,
                      const std::string& seKey,
                      int bpm,
                      int division = static_cast<int>(GridDivision::Sixteenth))
        : m_bgmKey(bgmKey), m_seKey(seKey), m_bpm(bpm), m_division(division) {}

    void SetBpm(int bpm) { m_bpm = bpm; }
    void SetDivision(int div) { m_division = div; }        // 4 = 16分, 8 = 32分
    void SetManualOffsetMs(double offsetMs) { m_offsetMs = offsetMs; }

    // 入力発生時に呼ぶ。「次のグリッド」で鳴らす要求をセット
    void RequestFire(float volume = 1.0f)
    {
        const double tMs = SoundManager::GetPlaybackTimeMilliseconds(m_bgmKey, true);
        m_targetGridIndex = static_cast<int64_t>(std::llround((tMs + m_offsetMs) / GetGridMs()));
        m_pendingVolume = volume;
        m_pending = true;
    }

    // 毎フレーム呼ぶ。オーディオ時間がターゲットグリッドに到達したら再生
    void Update()
    {
        if (!m_pending) return;

        const double tMs = SoundManager::GetPlaybackTimeMilliseconds(m_bgmKey, true);
        const int64_t nowGrid = GetCurrentGrid(tMs);

        if (nowGrid >= m_targetGridIndex && m_targetGridIndex != m_lastFiredGrid)
        {
            m_lastFiredGrid = m_targetGridIndex;
            m_pending = false;
            SoundManager::Play(m_seKey, false, m_pendingVolume);
        }
    }

private:
    double GetGridMs() const
    {
        return 60000.0 / static_cast<double>(m_bpm) / static_cast<double>(m_division);
    }

    int64_t GetCurrentGrid(double tMs) const
    {
        return static_cast<int64_t>(std::floor((tMs + m_offsetMs) / GetGridMs()));
    }

    std::string m_bgmKey;
    std::string m_seKey;
    int m_bpm = 120;
    int m_division = static_cast<int>(GridDivision::Sixteenth);            // 4=16分, 8=32分
    double m_offsetMs = 0.0;       // 体感オフセット調整用
    bool m_pending = false;
    int64_t m_targetGridIndex = -1;
    int64_t m_lastFiredGrid = -1;
    float m_pendingVolume = 1.0f;
};