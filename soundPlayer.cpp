#include "soundPlayer.h"
#include "soundManager.h"
#include <cstring>
#include <algorithm>

//=========================================================
// 内部ヘルパ関数
//=========================================================
namespace
{
    //---------------------------------
    // フォーマットから1秒あたりのバイト数を解決する
    // nAvgBytesPerSec が 0 のフォーマットに対応するためフォールバックを持つ
    //---------------------------------
    uint32_t ResolveBytesPerSecond(const WAVEFORMATEX& format)
    {
        if (format.nAvgBytesPerSec > 0)
            return format.nAvgBytesPerSec;

        // nAvgBytesPerSec が未設定のフォーマット向けフォールバック
        return format.nSamplesPerSec * std::max<uint16_t>(1, format.nBlockAlign);
    }

    //---------------------------------
    // 再生開始秒数をブロックアライン済みバイトオフセットに変換する
    //
    // XAudio2 は nBlockAlign の倍数でないオフセットを受け付けないため
    // 切り捨て処理が必須。中途半端な位置から再生するとノイズが発生する。
    // chunkSize を超えた場合は末尾直前にクランプする。
    //---------------------------------
    uint32_t CalcStartBytes(double startTimeSeconds,
                            const WAVEFORMATEX& format,
                            uint32_t chunkSize)
    {
        const uint32_t align          = std::max<uint16_t>(1, format.nBlockAlign);
        const uint32_t bytesPerSecond = ResolveBytesPerSecond(format);

        uint64_t startBytes = static_cast<uint64_t>(
            std::max(0.0, startTimeSeconds) * bytesPerSecond);

        // ブロックアライン境界に切り捨て
        startBytes -= startBytes % align;

        // チャンクサイズ超過時のクランプ
        if (startBytes >= chunkSize)
        {
            startBytes = (chunkSize > align)
                ? (chunkSize - align) - ((chunkSize - align) % align)
                : 0;
        }

        return static_cast<uint32_t>(startBytes);
    }

    //---------------------------------
    // X3DAUDIO_EMITTER をデフォルト状態に初期化する
    // ChannelCount=1（モノラル発音源）を前提とした設定。
    // マルチチャンネルエミッターを使う場合は ChannelRadius も設定が必要。
    //---------------------------------
    void InitEmitter(X3DAUDIO_EMITTER& emitter)
    {
        std::memset(&emitter, 0, sizeof(emitter));
        emitter.OrientFront  = X3DAUDIO_VECTOR(0.0f, 0.0f, 1.0f); // 前方向
        emitter.OrientTop    = X3DAUDIO_VECTOR(0.0f, 1.0f, 0.0f); // 上方向
        emitter.ChannelCount = 1;                                  // モノラル発音源
    }

    //---------------------------------
    // X3DAUDIO_DSP_SETTINGS をデフォルト状態に初期化する
    // pMatrixCoefficients は呼び出し元の配列を指す必要がある（ポインタ渡し）
    //---------------------------------
    void InitDspSettings(X3DAUDIO_DSP_SETTINGS& dsp, float* matrixCoefficients)
    {
        std::memset(&dsp, 0, sizeof(dsp));
        dsp.SrcChannelCount     = 1;                 // 入力：モノラル
        dsp.DstChannelCount     = 2;                 // 出力：ステレオ
        dsp.pMatrixCoefficients = matrixCoefficients; // 左右音量配分先
    }

} // namespace


//=========================================================
// コンストラクタ・デストラクタ
//=========================================================

//---------------------------------
// SE用コンストラクタ（メモリ全体読み込み音源）
// SoundData のポインタを保持するだけで、データ本体は SoundLoader が管理する
//---------------------------------
SoundPlayer::SoundPlayer(IXAudio2* xAudio2, const SoundData* soundData)
    : m_sourceVoice(nullptr)
    , m_soundData(soundData)
    , m_streamData(nullptr)
{
    if (xAudio2 && m_soundData)
    {
        // this を渡すことで OnStreamEnd などのコールバックを自身で受け取る
        xAudio2->CreateSourceVoice(&m_sourceVoice, &m_soundData->format, 0, 2.0f, this);
    }

    InitEmitter(m_emitter);
    InitDspSettings(m_dspSettings, m_matrixCoefficients);
}

//---------------------------------
// BGM用コンストラクタ（ストリーミング音源）
// StreamingSoundData はファイルパスとオフセットのみ保持し、
// 実データは StreamingFileReader が再生時に都度ファイルから読み込む
//---------------------------------
SoundPlayer::SoundPlayer(IXAudio2* xAudio2, const StreamingSoundData* streamData)
    : m_sourceVoice(nullptr)
    , m_soundData(nullptr)
    , m_streamData(streamData)
    , m_isStreaming(true)
{
    if (xAudio2 && m_streamData)
    {
        xAudio2->CreateSourceVoice(&m_sourceVoice, &m_streamData->format, 0, 2.0f, this);
    }

    // ストリーミングバッファを事前確保
    // STREAMING_BUFFER_COUNT 個のバッファをローテーションして途切れを防ぐ
    for (int i = 0; i < STREAMING_BUFFER_COUNT; ++i)
    {
        m_streamBuffers[i].resize(STREAMING_BUFFER_SIZE);
        m_streamBufferSizes[i] = 0;
    }

    InitEmitter(m_emitter);
    InitDspSettings(m_dspSettings, m_matrixCoefficients);
}

//---------------------------------
// デストラクタ
// SourceVoice は必ず DestroyVoice() で明示解放する必要がある（delete 不可）
// ファイルも先に閉じてから Voice を破棄する
//---------------------------------
SoundPlayer::~SoundPlayer()
{
    m_fileReader.Close();

    if (m_sourceVoice)
    {
        m_sourceVoice->DestroyVoice();
        m_sourceVoice = nullptr;
    }
}


//=========================================================
// メモリ再生（SE）
//=========================================================

//---------------------------------
// 再生開始
// 音声データはメモリ上に全て存在するため、バッファを1つ投入するだけで完結する
//---------------------------------
void SoundPlayer::Play(bool loop, double startTimeSeconds)
{
    if (!m_sourceVoice || !m_soundData) return;

    m_sourceVoice->Stop();
    m_sourceVoice->FlushSourceBuffers();

    const uint32_t startBytes = CalcStartBytes(
        startTimeSeconds, m_soundData->format,
        static_cast<uint32_t>(m_soundData->audioData.size()));

    const size_t totalBytes = m_soundData->audioData.size();
    if (totalBytes == 0 || startBytes >= totalBytes) return;

    XAUDIO2_BUFFER buffer{};
    buffer.pAudioData = m_soundData->audioData.data() + startBytes;
    buffer.AudioBytes = static_cast<UINT32>(totalBytes - startBytes);
    buffer.Flags      = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount  = loop ? XAUDIO2_LOOP_INFINITE : 0;

    m_sourceVoice->SubmitSourceBuffer(&buffer);
    m_sourceVoice->Start();
}


//=========================================================
// ストリーミング再生（BGM）
//=========================================================

//---------------------------------
// ストリーミング再生開始
// STREAMING_BUFFER_COUNT 個のバッファを先読みしてキューに投入し、
// 以降は UpdateStreaming() が毎フレーム空きバッファを補充することで
// メモリに全データを乗せずに途切れなく再生し続ける
//---------------------------------
void SoundPlayer::PlayStreaming(bool loop, double startTimeSeconds)
{
    if (!m_sourceVoice || !m_streamData) return;

    m_sourceVoice->Stop();
    m_sourceVoice->FlushSourceBuffers();

    const uint32_t startBytes = CalcStartBytes(
        startTimeSeconds, m_streamData->format, m_streamData->dataChunkSize);

    // StreamingFileReader にファイルI/Oを委譲
    // SoundPlayer はファイルの位置や読み取りを直接操作しない
    if (!m_fileReader.Open(*m_streamData, startBytes)) return;

    m_streamEnded        = false;
    m_isLooping          = loop;
    m_currentBufferIndex = 0;

    // バッファを先読みしてキューイング（トリプルバッファリング）
    // [バッファ0: 再生中] [バッファ1: 待機] [バッファ2: 補充待ち]
    for (int i = 0; i < STREAMING_BUFFER_COUNT; ++i)
    {
        if (LoadNextStreamBuffer(i))
            SubmitStreamBuffer(i);
    }

    m_sourceVoice->Start();
}

//---------------------------------
// ストリーミングバッファ補充（毎フレーム呼び出し）
// XAudio2 は非同期で再生するため、再生中に次のバッファを補充しなければ音が途切れる
// キューの空き数が閾値を下回ったときだけ補充する
//---------------------------------
void SoundPlayer::UpdateStreaming()
{
    if (!m_isStreaming || !m_sourceVoice) return;

    XAUDIO2_VOICE_STATE state;
    m_sourceVoice->GetState(&state);

    if (state.BuffersQueued < STREAMING_BUFFER_COUNT && !m_streamEnded)
    {
        if (LoadNextStreamBuffer(m_currentBufferIndex))
        {
            SubmitStreamBuffer(m_currentBufferIndex);
            // 0→1→2→0→... とインデックスをローテーション
            m_currentBufferIndex = (m_currentBufferIndex + 1) % STREAMING_BUFFER_COUNT;
        }
    }
}

//---------------------------------
// ファイルから次のバッファを読み込む（内部処理）
// ループ時はファイル終端で SeekToStart() を呼んで巻き戻す
// 非ループ時は終端到達で m_streamEnded を立て、以降の補充を止める
//---------------------------------
bool SoundPlayer::LoadNextStreamBuffer(int bufferIndex)
{
    if (!m_fileReader.IsOpen() || m_streamEnded) return false;

    if (m_fileReader.IsEndOfData())
    {
        if (m_isLooping)
            m_fileReader.SeekToStart();
        else
        {
            m_streamEnded = true;
            return false;
        }
    }

    const uint32_t bytesRead = m_fileReader.ReadChunk(
        m_streamBuffers[bufferIndex], STREAMING_BUFFER_SIZE);

    if (bytesRead == 0)
    {
        m_streamEnded = true;
        return false;
    }

    m_streamBufferSizes[bufferIndex] = bytesRead;
    return true;
}

//---------------------------------
// バッファを XAudio2 のキューに投入する（内部処理）
// 非ループかつ最終バッファの場合は XAUDIO2_END_OF_STREAM フラグを立て
// XAudio2 に再生終了を通知する（→ OnStreamEnd コールバックが呼ばれる）
//---------------------------------
void SoundPlayer::SubmitStreamBuffer(int bufferIndex)
{
    if (!m_sourceVoice) return;

    XAUDIO2_BUFFER buffer{};
    buffer.pAudioData = m_streamBuffers[bufferIndex].data();
    buffer.AudioBytes = m_streamBufferSizes[bufferIndex];

    if (!m_isLooping && m_fileReader.IsEndOfData())
        buffer.Flags = XAUDIO2_END_OF_STREAM;

    m_sourceVoice->SubmitSourceBuffer(&buffer);
}


//=========================================================
// 再生コントロール
//=========================================================

void SoundPlayer::Stop()
{
    if (m_sourceVoice)
    {
        m_sourceVoice->Stop();
        m_sourceVoice->FlushSourceBuffers(); // キューを空にしないと再利用時に古いデータが残る
    }
}

void SoundPlayer::Pause()
{
    // Stop() は再生位置を保持したまま停止する（FlushSourceBuffers を呼ばない）
    if (m_sourceVoice) m_sourceVoice->Stop();
}

void SoundPlayer::Resume()
{
    if (m_sourceVoice) m_sourceVoice->Start();
}

bool SoundPlayer::IsPlaying() const
{
    if (!m_sourceVoice) return false;
    XAUDIO2_VOICE_STATE state;
    m_sourceVoice->GetState(&state);
    // バッファキューが空になれば再生終了と判断する
    return state.BuffersQueued > 0;
}


//=========================================================
// ボリューム・ピッチ
//=========================================================

void SoundPlayer::SetVolume(float volume)
{
    if (m_sourceVoice) m_sourceVoice->SetVolume(volume);
}

void SoundPlayer::SetPitch(float pitch)
{
    // SetFrequencyRatio: 1.0f = 等速、2.0f = 1オクターブ上
    if (m_sourceVoice) m_sourceVoice->SetFrequencyRatio(pitch);
}


//=========================================================
// 再生時間取得
//=========================================================

//---------------------------------
// XAudio2 が内部で管理するサンプル数を取得する
// OS タイマーより精度が高く、BeatManager での BPM 同期計算に使用される
//---------------------------------
uint64_t SoundPlayer::GetSamplesPlayed() const
{
    if (!m_sourceVoice) return 0ULL;
    XAUDIO2_VOICE_STATE state;
    m_sourceVoice->GetState(&state, 0);
    return state.SamplesPlayed;
}

double SoundPlayer::GetPlaybackTimeSeconds() const
{
    const WAVEFORMATEX* format = GetFormat();
    if (!format || format->nSamplesPerSec == 0) return 0.0;
    // サンプル数 ÷ サンプルレート = 経過秒数
    return static_cast<double>(GetSamplesPlayed()) / format->nSamplesPerSec;
}

double SoundPlayer::GetPlaybackTimeMilliseconds() const
{
    return GetPlaybackTimeSeconds() * 1000.0;
}


//=========================================================
// 3Dオーディオ
//=========================================================

void SoundPlayer::SetPosition(const Vector3& pos)
{
    m_position         = pos;
    m_emitter.Position = X3DAUDIO_VECTOR(pos.x, pos.y, pos.z);
}

void SoundPlayer::SetVelocity(const Vector3& vel)
{
    m_velocity         = vel;
    m_emitter.Velocity = X3DAUDIO_VECTOR(vel.x, vel.y, vel.z);
}

void SoundPlayer::Enable3D(bool enable)
{
    m_is3DEnabled = enable;
}

//---------------------------------
// 3D空間定位の更新
// リスナー（耳）とエミッター（音源）の位置・向きから
// 左右スピーカーの音量比率とドップラーピッチ変化を毎フレーム計算する
//---------------------------------
void SoundPlayer::Update3DAudio(const X3DAUDIO_LISTENER& listener)
{
    if (!m_sourceVoice || !m_is3DEnabled) return;

    X3DAudioCalculate(
        SoundManager::GetX3DInstance(),
        &listener,
        &m_emitter,
        X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER,
        &m_dspSettings);

    // 距離・方向に応じた左右音量配分を反映
    m_sourceVoice->SetOutputMatrix(
        nullptr,
        m_dspSettings.SrcChannelCount,
        m_dspSettings.DstChannelCount,
        m_dspSettings.pMatrixCoefficients);

    // 音源との相対速度によるドップラー効果を反映
    m_sourceVoice->SetFrequencyRatio(m_dspSettings.DopplerFactor);
}


//=========================================================
// IXAudio2VoiceCallback の実装
// XAudio2 内部スレッドから呼ばれる。処理は最小限に留める
//=========================================================

void SoundPlayer::OnBufferEnd(void*)
{
    // 現状は UpdateStreaming() のポーリングでバッファ補充を行うため何もしない
    // 将来：即時補充が必要な場合はここでフラグを立てる
}

//---------------------------------
// 全バッファの再生が完了したときに呼ばれる
// m_streamEnded を立てることで UpdateStreaming() の補充を停止する
//---------------------------------
void SoundPlayer::OnStreamEnd()
{
    m_streamEnded = true;
}


//=========================================================
// プライベートヘルパ
//=========================================================

//---------------------------------
// 現在のフォーマット情報を返す
// メモリ再生・ストリーミング再生の両方に対応するための共通アクセサ
//---------------------------------
const WAVEFORMATEX* SoundPlayer::GetFormat() const
{
    if (m_soundData)  return &m_soundData->format;
    if (m_streamData) return &m_streamData->format;
    return nullptr;
}