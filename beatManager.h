//=========================================================
// beatManager.h
// ビート管理クラス
//
// BPM と音声再生時間を元に現在のビート位置を管理する。
// SoundManager からオーディオ再生時間を取得できる場合はそれを優先し、
// 取得できない場合は固定ステップ積算でタイムラインを推進する。
// シングルトンとして使用し、GetInstance() でアクセスする。
//=========================================================
#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include "gameObject.h"

class BeatManager : public GameObject
{
public:
    BeatManager() = default;

    //-----------------------------------------------------
    // ライフサイクル
    //-----------------------------------------------------
    void Init();
    void Uninit() override;
    void Update() override;

    //-----------------------------------------------------
    // シングルトンアクセサ
    //-----------------------------------------------------
    static BeatManager* GetInstance() { return s_instance; }

    //-----------------------------------------------------
    // BPM 設定・取得
    // SetBPM に 1 未満の値を渡した場合は 1 にクランプする
    //-----------------------------------------------------
    void SetBPM(int bpm);
    int GetBPM() const { return m_bpm; }

    //-----------------------------------------------------
    // 追跡するサウンドキー
    // SoundManager に登録されたキーを指定することで、
    // 実際の再生時間をビート計算の基準にできる
    //-----------------------------------------------------
    void SetTrackedSoundKey(const std::string& key, bool searchStreaming = true);
    const std::string& GetTrackedSoundKey() const { return m_trackedSoundKey; }

    //-----------------------------------------------------
    // 手動オフセット
    // ハードウェアや曲データのズレを補正するために使用する（単位: ms）
    //-----------------------------------------------------
    void SetManualOffsetMilliseconds(double offsetMs);
    double GetManualOffsetMilliseconds() const { return m_manualOffsetMs; }

    //-----------------------------------------------------
    // 現在の再生時間取得
    //-----------------------------------------------------
    double GetCurrentTimeMilliseconds() const { return m_currentTimeMs; }
    double GetCurrentTimeSeconds() const { return m_currentTimeMs * 0.001; }

    //-----------------------------------------------------
    // ビート位置クエリ
    // GetBeatIntervalMilliseconds : 1ビートあたりの時間（ms）
    // GetBeatsSinceStart          : 開始からの経過ビート数（小数含む）
    // GetCurrentBeatIndex         : 現在のビート番号（整数、0始まり）
    // GetBeatPhase                : ビート内の位相（0.0〜1.0）
    // GetTimeUntilNextBeatMilliseconds : 次のビートまでの残り時間（ms）
    //-----------------------------------------------------
    double GetBeatIntervalMilliseconds() const;
    double GetBeatsSinceStart() const;
    uint64_t GetCurrentBeatIndex() const;
    double GetBeatPhase() const;
    double GetTimeUntilNextBeatMilliseconds() const;

    //-----------------------------------------------------
    // オーディオロック状態
    // true の場合、オーディオ時間を基準にビートを計算している
    //-----------------------------------------------------
    bool HasAudioLock() const { return m_hasAudioLock; }

    //-----------------------------------------------------
    // タイムラインリセット
    // シーン遷移や曲の切り替え時に外部から呼び出す
    //-----------------------------------------------------
    void ForceResetTimeline();

private:
    using Clock = std::chrono::steady_clock;

    static BeatManager* s_instance;

    //-----------------------------------------------------
    // サウンド追跡
    //-----------------------------------------------------
    std::string m_trackedSoundKey;
    bool m_searchStreaming = true; // ストリーミング再生を検索対象にするか

    //-----------------------------------------------------
    // タイムライン状態
    //-----------------------------------------------------
    int    m_bpm           = 120;
    double m_manualOffsetMs = 0.0;  // 手動補正オフセット（ms）
    double m_currentTimeMs  = 0.0;  // 現在の再生時間（ms）
    double m_accumulatorMs{ 0.0 };  // 固定ステップ積算用アキュムレータ
    bool   m_hasAudioLock   = false; // オーディオ時間を取得できているか
    double m_seekOffsetMs   = 0.0;  // シーク時のオフセット補正（ms）

    Clock::time_point m_lastUpdateTime{};

    //-----------------------------------------------------
    // プライベートヘルパ
    // ResetTimeline          : タイムラインを初期状態に戻す
    // QueryAudioTimeMilliseconds : SoundManager から再生時間を取得する
    //-----------------------------------------------------
    void ResetTimeline();
    double QueryAudioTimeMilliseconds() const;
};
