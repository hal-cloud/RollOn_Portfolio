//=========================================================
// soundPlayer.h
// サウンド再生クラス
//
// SoundLoader で読み込んだ音声データを XAudio2 SourceVoice で再生する。
// SE 向けのメモリ再生と BGM 向けのストリーミング再生の両方に対応する。
// IXAudio2VoiceCallback を継承し、バッファ終端などのイベントを自身で受け取る。
//=========================================================
#pragma once
#define NOMINMAX // Windows.h の min/max マクロを無効化（std::min/std::max を使うため）
#include "main.h"
#include <xaudio2.h>
#include <X3DAudio.h>
#include <cstdint>
#include "soundLoader.h"
#include "streamingFileReader.h"


class SoundPlayer : public IXAudio2VoiceCallback
{
public:
    //-----------------------------------------------------
    // コンストラクタ・デストラクタ
    // SE 用（SoundData）と BGM 用（StreamingSoundData）で別々に構築する
    //-----------------------------------------------------
    SoundPlayer(IXAudio2* xAudio2, const SoundData* soundData);
    SoundPlayer(IXAudio2* xAudio2, const StreamingSoundData* streamData);
    ~SoundPlayer();

    //-----------------------------------------------------
    // 再生コントロール
    // Play          : メモリ全体再生（SE 向け）
    // PlayStreaming  : ファイルから逐次読み込みながら再生（BGM 向け）
    // UpdateStreaming: 毎フレーム呼び出し、バッファを補充してストリーミングを継続する
    //-----------------------------------------------------
    void Play(bool loop = false, double startTimeSeconds = 0.0);
    void PlayStreaming(bool loop = false, double startTimeSeconds = 0.0);
    void Stop();
    void Pause();
    void Resume();
    bool IsPlaying() const;
    void UpdateStreaming();

    //-----------------------------------------------------
    // ボリューム・ピッチ
    // SetPitch: SetFrequencyRatio のラッパー（1.0f = 等速、2.0f = 1オクターブ上）
    //-----------------------------------------------------
    void SetVolume(float volume);
    void SetPitch(float pitch);

    //-----------------------------------------------------
    // 再生時間取得
    // GetSamplesPlayed は OS タイマーより高精度なため BeatManager の BPM 同期に使用する
    //-----------------------------------------------------
    uint64_t GetSamplesPlayed() const;
    double GetPlaybackTimeSeconds() const;
    double GetPlaybackTimeMilliseconds() const;

    //-----------------------------------------------------
    // 3D空間定位
    // Enable3D を true にすると Update3DAudio() で毎フレーム定位が更新される
    //-----------------------------------------------------
    void SetPosition(const Vector3& pos);
    void SetVelocity(const Vector3& vel);
    void Enable3D(bool enable);
    bool Is3DEnabled() const { return m_is3DEnabled; }
    void Update3DAudio(const X3DAUDIO_LISTENER& listener);

    //-----------------------------------------------------
    // IXAudio2VoiceCallback の実装
    // XAudio2 内部スレッドから呼ばれるため、処理は最小限に留める
    //-----------------------------------------------------
    void OnBufferEnd(void* pBufferContext) override;
    void OnStreamEnd() override;
    void OnVoiceProcessingPassStart(UINT32) override {}
    void OnVoiceProcessingPassEnd() override {}
    void OnBufferStart(void*) override {}
    void OnLoopEnd(void*) override {}
    void OnVoiceError(void*, HRESULT) override {}

private:
    //-----------------------------------------------------
    // XAudio2 リソース
    // m_sourceVoice は必ず DestroyVoice() で解放する（delete 不可）
    // m_soundData / m_streamData はポインタを借りるだけで所有権は SoundLoader が持つ
    //-----------------------------------------------------
    IXAudio2SourceVoice*      m_sourceVoice = nullptr;
    const SoundData*          m_soundData   = nullptr; // SE 用データ（非所有）
    const StreamingSoundData* m_streamData  = nullptr; // BGM 用データ（非所有）

    //-----------------------------------------------------
    // 再生状態フラグ
    //-----------------------------------------------------
    bool m_is3DEnabled = false; // 3D空間定位を有効にするか
    bool m_isStreaming  = false; // ストリーミング再生モードか
    bool m_isLooping    = false; // ループ再生か
    bool m_streamEnded  = false; // ストリーミングデータを読み終えたか

    //-----------------------------------------------------
    // ストリーミングバッファ（トリプルバッファリング）
    // 3つのバッファをローテーションして途切れなく補充し続ける。
    // STREAMING_BUFFER_SIZE はバッファ1個あたりのバイト数。
    // 小さすぎると補充頻度が上がり、大きすぎるとメモリを浪費する。
    //-----------------------------------------------------
    static const int STREAMING_BUFFER_COUNT = 3;
    static const int STREAMING_BUFFER_SIZE  = 65536; // 64KB × 3バッファ
    std::vector<BYTE> m_streamBuffers[STREAMING_BUFFER_COUNT];
    uint32_t          m_streamBufferSizes[STREAMING_BUFFER_COUNT]{};
    uint32_t          m_currentBufferIndex = 0; // 次に書き込むバッファの番号

    // ファイルI/Oの責務を分離したクラス（再生とファイル操作を混在させない）
    StreamingFileReader m_fileReader;

    //-----------------------------------------------------
    // 3Dオーディオ計算用
    // m_matrixCoefficients は DSP 設定が指す配列。ポインタの寿命に注意。
    //-----------------------------------------------------
    X3DAUDIO_EMITTER      m_emitter{};
    X3DAUDIO_DSP_SETTINGS m_dspSettings{};
    float                 m_matrixCoefficients[8]{}; // 左右スピーカーへの音量配分
    Vector3               m_position{};
    Vector3               m_velocity{};

    //-----------------------------------------------------
    // プライベートヘルパ
    //-----------------------------------------------------
    const WAVEFORMATEX* GetFormat() const; // メモリ・ストリーミング両方に対応した共通アクセサ
    bool LoadNextStreamBuffer(int bufferIndex);
    void SubmitStreamBuffer(int bufferIndex);
};