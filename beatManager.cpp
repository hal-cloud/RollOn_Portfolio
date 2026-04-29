//=========================================================
// beatManager.cpp
// ビート管理クラス実装
//=========================================================
#include "beatManager.h"
#include "soundManager.h"

#include <algorithm>
#include <cmath>

//=========================================================
// 内部定数
//=========================================================
namespace
{
    // 固定ステップ積算の刻み幅（ms）
    // 小さいほど精度が上がるが、ループ回数が増える
    constexpr double kFixedStepMs = 5.0;

    // 1フレームあたりの最大デルタ時間（ms）
    // スパイクによるタイムラインの過剰な進みを防ぐ
    constexpr double kMaxDeltaMs  = 100.0;
}

BeatManager* BeatManager::s_instance = nullptr;

//=========================================================
// ライフサイクル
//=========================================================

void BeatManager::Init()
{
    s_instance = this;
    m_lastUpdateTime = Clock::now();
    m_currentTimeMs = 0.0;
    m_accumulatorMs = 0.0;
    m_hasAudioLock = false;
    m_seekOffsetMs = 0.0;
}

void BeatManager::Uninit()
{
    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

//-----------------------------------------------------
// 毎フレーム更新
// オーディオ時間が取得できる場合は直接同期し、
// 取得できない場合は固定ステップで積算してタイムラインを推進する。
// 固定ステップ方式はフレームレート変動の影響を受けにくい。
//-----------------------------------------------------
void BeatManager::Update()
{
    GameObject::Update();

    const auto now = Clock::now();
    double deltaMs = std::chrono::duration<double, std::milli>(now - m_lastUpdateTime).count();
    m_lastUpdateTime = now;

    deltaMs = std::min(deltaMs, kMaxDeltaMs);

    const double audioTime = QueryAudioTimeMilliseconds();
    if (audioTime >= 0.0)
    {
        // オーディオ時間で直接同期（最優先）
        m_currentTimeMs = std::max(0.0, audioTime + m_manualOffsetMs);
        m_accumulatorMs = 0.0;
        m_hasAudioLock = true;
        return;
    }

    // オーディオ時間が取得できない場合は固定ステップで積算
    m_accumulatorMs += deltaMs;
    while (m_accumulatorMs >= kFixedStepMs)
    {
        m_currentTimeMs = std::max(0.0, m_currentTimeMs + kFixedStepMs);
        m_accumulatorMs -= kFixedStepMs;
    }
    m_hasAudioLock = false;
}

//=========================================================
// BPM 設定
//=========================================================

void BeatManager::SetBPM(int bpm)
{
    m_bpm = std::max(1, bpm); // 0以下は不正値のため 1 にクランプ
}

//=========================================================
// サウンドキー設定
//=========================================================

//-----------------------------------------------------
// 追跡するサウンドキーを設定してタイムラインをリセットする
// 曲の切り替え時に呼び出し、前の曲の時間が残らないようにする
//-----------------------------------------------------
void BeatManager::SetTrackedSoundKey(const std::string& key, bool searchStreaming)
{
    m_trackedSoundKey = key;
    m_searchStreaming = searchStreaming;
    ResetTimeline();
}

//=========================================================
// オフセット設定
//=========================================================

void BeatManager::SetManualOffsetMilliseconds(double offsetMs)
{
    m_manualOffsetMs = offsetMs;
}

//=========================================================
// ビート位置クエリ
//=========================================================

//-----------------------------------------------------
// 1ビートあたりの時間を返す（ms）
// BPM が 0 以下の場合は 0.0 を返してゼロ除算を防ぐ
//-----------------------------------------------------
double BeatManager::GetBeatIntervalMilliseconds() const
{
    if (m_bpm <= 0)
    {
        return 0.0;
    }
    return 60000.0 / static_cast<double>(m_bpm);
}

//-----------------------------------------------------
// 開始からの経過ビート数を返す（小数含む）
//-----------------------------------------------------
double BeatManager::GetBeatsSinceStart() const
{
    const double interval = GetBeatIntervalMilliseconds();
    return interval > 0.0 ? m_currentTimeMs / interval : 0.0;
}

//-----------------------------------------------------
// 現在のビート番号を返す（0始まり整数）
//-----------------------------------------------------
uint64_t BeatManager::GetCurrentBeatIndex() const
{
    return static_cast<uint64_t>(GetBeatsSinceStart());
}

//-----------------------------------------------------
// ビート内の位相を返す（0.0〜1.0）
// 0.0 がビートの瞬間、1.0 に近いほど次のビートに近い
//-----------------------------------------------------
double BeatManager::GetBeatPhase() const
{
    double integral = 0.0;
    const double fractional = std::modf(GetBeatsSinceStart(), &integral);
    return fractional < 0.0 ? fractional + 1.0 : fractional;
}

//-----------------------------------------------------
// 次のビートまでの残り時間を返す（ms）
//-----------------------------------------------------
double BeatManager::GetTimeUntilNextBeatMilliseconds() const
{
    const double interval = GetBeatIntervalMilliseconds();
    if (interval <= 0.0)
    {
        return 0.0;
    }

    double remainder = std::fmod(m_currentTimeMs, interval);
    if (remainder < 0.0)
    {
        remainder += interval;
    }
    return interval - remainder;
}

//=========================================================
// タイムラインリセット
//=========================================================

void BeatManager::ForceResetTimeline()
{
    ResetTimeline();
}

//-----------------------------------------------------
// タイムラインを初期状態に戻す
// m_seekOffsetMs もリセットすることで、シーク後の誤差が累積しない
//-----------------------------------------------------
void BeatManager::ResetTimeline()
{
    m_currentTimeMs = 0.0;
    m_lastUpdateTime = Clock::now();
    m_hasAudioLock = false;
    m_seekOffsetMs = 0.0;
}

//=========================================================
// プライベートヘルパ
//=========================================================

//-----------------------------------------------------
// SoundManager からオーディオ再生時間を取得する
// サウンドが未登録またはまだ再生されていない場合は -1.0 を返す。
// 呼び出し元でこの戻り値をチェックし、固定ステップとの切り替えを行う。
//-----------------------------------------------------
double BeatManager::QueryAudioTimeMilliseconds() const
{
    if (m_trackedSoundKey.empty())
    {
        return -1.0;
    }

    const double playbackMs = SoundManager::GetPlaybackTimeMilliseconds(m_trackedSoundKey, m_searchStreaming);
    const bool active = SoundManager::IsSoundActive(m_trackedSoundKey, m_searchStreaming);

    if (!active && playbackMs <= 0.0)
    {
        return -1.0;
    }

    return playbackMs + m_seekOffsetMs;
}
