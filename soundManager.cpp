#include "soundManager.h"
#include <random>
#include <algorithm>

//=========================================================
// 内部ヘルパ関数・定数
// このファイル内でのみ使用するため無名 namespace に閉じ込める
//=========================================================
namespace
{
    // 各キーごとにアイドル状態で保持するプレイヤーの最大数
    // 多すぎるとメモリ浪費、少なすぎると連打時に生成コストが発生する
    constexpr size_t kMaxIdlePlayersPerKey = 8;

    // 同一キーの最小再生間隔（ms）
    // 連打による大量生成・ノイズを防ぐ
    constexpr long long kMinPlayIntervalMs = 30;

    // マネージャ側で初期化した COM の対応を管理
    bool g_comInitializedBySoundManager = false;

    // 二重初期化防止
    bool g_isSoundManagerInitialized = false;

    // キーごとに複数プレイヤーを管理するマップの型エイリアス
    using PlayerMap = std::unordered_map<std::string, std::vector<std::unique_ptr<SoundPlayer>>>;

    //-----------------------------------------------------
    // 乱数生成器（static ローカルで1度だけ初期化）
    //
    // random_device はシステムコールを伴うため毎回生成するとコストが高い。
    // mt19937 を static で保持することで再利用し、生成コストを1回に抑える。
    //-----------------------------------------------------
    std::mt19937& GetRng()
    {
        static std::mt19937 s_rng{ std::random_device{}() };
        return s_rng;
    }

    //-----------------------------------------------------
    // 指定キーの再生中プレイヤーを検索する
    //
    // const PlayerMap& を受け取ることで読み取り専用であることを明示する。
    // 変更を加えないのに非 const 参照を受け取るのは設計上の誤りのため修正。
    //-----------------------------------------------------
    SoundPlayer* FindActivePlayer(const PlayerMap& players, const std::string& key)
    {
        auto it = players.find(key);
        if (it == players.end()) return nullptr;

        for (const auto& player : it->second)
        {
            if (player && player->IsPlaying())
                return player.get();
        }
        return nullptr;
    }

    //-----------------------------------------------------
    // 再生終了済みプレイヤーを再利用し、なければ新規作成する（プールパターン）
    //
    // 毎回 new/delete するとメモリ断片化・生成コストが発生するため
    // 再生終了済みのプレイヤーを再利用する。
    // プールに空きがなければ新規作成してリストに追加する。
    //-----------------------------------------------------
    SoundPlayer* AcquireReusablePlayer(std::vector<std::unique_ptr<SoundPlayer>>& list,
                                       const SoundData* data,
                                       IXAudio2* xAudio2)
    {
        for (auto& p : list)
        {
            if (p && !p->IsPlaying())
                return p.get();
        }

        list.push_back(std::make_unique<SoundPlayer>(xAudio2, data));
        return list.back().get();
    }

    //-----------------------------------------------------
    // 同一キーの最小再生間隔チェック
    //
    // ループ再生は連打とならないため間隔チェックを免除する。
    // 非ループ再生が kMinPlayIntervalMs 以内に連続した場合は弾く。
    //-----------------------------------------------------
    bool CheckAndUpdatePlayInterval(
        std::unordered_map<std::string, std::chrono::steady_clock::time_point>& lastPlayTime,
        const std::string& key,
        bool loop)
    {
        if (loop) return true;

        const auto now = std::chrono::steady_clock::now();
        auto it = lastPlayTime.find(key);
        if (it != lastPlayTime.end())
        {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
            if (elapsed.count() < kMinPlayIntervalMs) return false;
        }
        lastPlayTime[key] = now;
        return true;
    }

    //-----------------------------------------------------
    // PlayerMap 内の全プレイヤーを停止してクリアする（共通処理）
    //
    // StopAll / Uninit で同じパターンを繰り返していたため共通化する。
    //-----------------------------------------------------
    void StopAndClearMap(PlayerMap& map)
    {
        for (auto& pair : map)
        {
            for (auto& player : pair.second)
            {
                if (player) player->Stop();
            }
        }
        map.clear();
    }

    //-----------------------------------------------------
    // ランダムなボリューム・ピッチ値を生成する（共通処理）
    //
    // Play_RandVP / Play3D_RandVP で同じ乱数生成コードが重複していたため
    // 共通関数に切り出す。
    //-----------------------------------------------------
    void ApplyRandomVP(SoundPlayer* player,
                       float baseVolume,
                       float randVolMin, float randVolMax,
                       float randPitMin, float randPitMax)
    {
        auto& rng = GetRng();
        std::uniform_real_distribution<float> volDist(randVolMin, randVolMax);
        std::uniform_real_distribution<float> pitDist(randPitMin, randPitMax);

        player->SetVolume(baseVolume * volDist(rng));
        player->SetPitch(pitDist(rng));
    }

    float ClampVolume(float volume)
    {
        if (volume < 0.0f) return 0.0f;
        if (volume > 1.0f) return 1.0f;
        return volume;
    }

} // namespace


//=========================================================
// 静的メンバーの定義
//=========================================================
Microsoft::WRL::ComPtr<IXAudio2>                                              SoundManager::s_xAudio2;
IXAudio2MasteringVoice*                                                       SoundManager::s_masterVoice   = nullptr;
SoundLoader                                                                   SoundManager::s_loader;
std::unordered_map<std::string, std::vector<std::unique_ptr<SoundPlayer>>>    SoundManager::s_players;
std::unordered_map<std::string, std::vector<std::unique_ptr<SoundPlayer>>>    SoundManager::s_streamingPlayers;
std::unordered_map<std::string, std::chrono::steady_clock::time_point>       SoundManager::s_lastPlayTime;
float                                                                         SoundManager::s_masterVolume  = 1.0f;
X3DAUDIO_HANDLE                                                               SoundManager::s_x3dInstance   = {};
X3DAUDIO_LISTENER                                                             SoundManager::s_listener       = {};
DWORD                                                                         SoundManager::s_channelMask    = 0;


//=========================================================
// 終了
//=========================================================
SoundManager::~SoundManager() { Uninit(); }

//-----------------------------------------------------
// XAudio2・X3DAudio・リスナーを初期化する
//
// CoInitializeEx はマルチスレッドモードで初期化する。
// X3DAudio のチャンネルマスクはマスターボイスのチャンネル数から取得し、
// モノ/ステレオを自動判定する。
//-----------------------------------------------------
bool SoundManager::Init()
{
    if (g_isSoundManagerInitialized)
        return true;

    const HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (comHr == RPC_E_CHANGED_MODE)
    {
        // 既に別モードで COM 初期化済み。ここでは解放責務を持たない。
        g_comInitializedBySoundManager = false;
    }
    else if (FAILED(comHr))
    {
        return false;
    }
    else
    {
        // S_OK / S_FALSE はどちらも CoUninitialize 対応が必要
        g_comInitializedBySoundManager = true;
    }

    if (FAILED(XAudio2Create(&s_xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
    {
        if (g_comInitializedBySoundManager)
        {
            CoUninitialize();
            g_comInitializedBySoundManager = false;
        }
        return false;
    }

    if (FAILED(s_xAudio2->CreateMasteringVoice(&s_masterVoice)))
    {
        s_xAudio2.Reset();
        if (g_comInitializedBySoundManager)
        {
            CoUninitialize();
            g_comInitializedBySoundManager = false;
        }
        return false;
    }

    // マスターボイスのチャンネル数からスピーカー構成を判定
    XAUDIO2_VOICE_DETAILS details;
    s_masterVoice->GetVoiceDetails(&details);
    s_channelMask = (details.InputChannels == 1) ? SPEAKER_MONO : SPEAKER_STEREO;

    if (FAILED(X3DAudioInitialize(s_channelMask, X3DAUDIO_SPEED_OF_SOUND, s_x3dInstance)))
    {
        s_masterVoice->DestroyVoice();
        s_masterVoice = nullptr;
        s_xAudio2.Reset();
        if (g_comInitializedBySoundManager)
        {
            CoUninitialize();
            g_comInitializedBySoundManager = false;
        }
        return false;
    }

    // リスナー（耳）の初期値：原点・Z前方・Y上方
    s_listener.Position    = X3DAUDIO_VECTOR(0.0f, 0.0f, 0.0f);
    s_listener.OrientFront = X3DAUDIO_VECTOR(0.0f, 0.0f, 1.0f);
    s_listener.OrientTop   = X3DAUDIO_VECTOR(0.0f, 1.0f, 0.0f);
    s_listener.Velocity    = X3DAUDIO_VECTOR(0.0f, 0.0f, 0.0f);

    s_masterVoice->SetVolume(ClampVolume(s_masterVolume));
    g_isSoundManagerInitialized = true;

    return true;
}

//-----------------------------------------------------
// 全リソースを解放する
//
// 解放順序が重要：
//   1. 全プレイヤーを Stop → Clear（SourceVoice への参照を先に切る）
//   2. SoundLoader をクリア（音声データを解放）
//   3. MasteringVoice を破棄
//   4. XAudio2 を解放
//   5. COM を終了
//-----------------------------------------------------
void SoundManager::Uninit()
{
    // SE・BGM 両方のプレイヤーを停止・破棄
    StopAndClearMap(s_players);
    StopAndClearMap(s_streamingPlayers);

    s_loader.Clear();

    if (s_masterVoice)
    {
        s_masterVoice->DestroyVoice();
        s_masterVoice = nullptr;
    }

    s_xAudio2.Reset();
    s_lastPlayTime.clear();

    if (g_comInitializedBySoundManager)
    {
        CoUninitialize();
        g_comInitializedBySoundManager = false;
    }

    g_isSoundManagerInitialized = false;
}


//=========================================================
// ロード
//=========================================================

bool SoundManager::Load(const std::string& key, const std::wstring& filename)
{
    return s_loader.Load(key, filename);
}

bool SoundManager::LoadStreaming(const std::string& key, const std::wstring& filename)
{
    return s_loader.LoadStreaming(key, filename);
}


//=========================================================
// 再生（メモリ音源・SE向け）
//=========================================================

//-----------------------------------------------------
// 通常の2D再生
//-----------------------------------------------------
void SoundManager::Play(const std::string& key, bool loop, float startVolume, double startTimeSeconds)
{
    const SoundData* data = s_loader.Get(key);
    if (!data || !s_xAudio2) return;
    if (!CheckAndUpdatePlayInterval(s_lastPlayTime, key, loop)) return;

    auto& list = s_players[key];
    SoundPlayer* player = AcquireReusablePlayer(list, data, s_xAudio2.Get());
    if (!player) return;

    player->SetVolume(ClampVolume(startVolume));
    player->Play(loop, startTimeSeconds);
}

//-----------------------------------------------------
// ランダムボリューム・ピッチ付き2D再生
// 足音・打撃音など同じ音が連続しても単調にならないよう
// 毎回わずかにボリューム・ピッチをランダム変動させる
//-----------------------------------------------------
void SoundManager::Play_RandVP(const std::string& key, bool loop, float startVolume,
    float randVolMax, float randVolMin, float randPitMax, float randPitMin, double startTimeSeconds)
{
    const SoundData* data = s_loader.Get(key);
    if (!data || !s_xAudio2) return;
    if (!CheckAndUpdatePlayInterval(s_lastPlayTime, key, loop)) return;

    auto& list = s_players[key];
    SoundPlayer* player = AcquireReusablePlayer(list, data, s_xAudio2.Get());
    if (!player) return;

    ApplyRandomVP(player, startVolume, randVolMin, randVolMax, randPitMin, randPitMax);
    player->Play(loop, startTimeSeconds);
}

//-----------------------------------------------------
// 3D空間定位付き再生
// X3DAudioCalculate によりリスナーとの距離・方向で音量・定位が変わる
//-----------------------------------------------------
void SoundManager::Play3D(const std::string& key, const Vector3& position, bool loop, float startVolume, double startTimeSeconds)
{
    const SoundData* data = s_loader.Get(key);
    if (!data || !s_xAudio2) return;
    if (!CheckAndUpdatePlayInterval(s_lastPlayTime, key, loop)) return;

    auto& list = s_players[key];
    SoundPlayer* player = AcquireReusablePlayer(list, data, s_xAudio2.Get());
    if (!player) return;

    player->SetVolume(ClampVolume(startVolume));
    player->SetPosition(position);
    player->Enable3D(true);
    player->Play(loop, startTimeSeconds);
}

//-----------------------------------------------------
// ランダムボリューム・ピッチ付き3D再生
//-----------------------------------------------------
void SoundManager::Play3D_RandVP(const std::string& key, const Vector3& position, bool loop, float startVolume,
    float randVolMax, float randVolMin, float randPitMax, float randPitMin, double startTimeSeconds)
{
    const SoundData* data = s_loader.Get(key);
    if (!data || !s_xAudio2) return;
    if (!CheckAndUpdatePlayInterval(s_lastPlayTime, key, loop)) return;

    auto& list = s_players[key];
    SoundPlayer* player = AcquireReusablePlayer(list, data, s_xAudio2.Get());
    if (!player) return;

    ApplyRandomVP(player, startVolume, randVolMin, randVolMax, randPitMin, randPitMax);
    player->SetPosition(position);
    player->Enable3D(true);
    player->Play(loop, startTimeSeconds);
}


//=========================================================
// 再生（ストリーミング・BGM向け）
//=========================================================

//-----------------------------------------------------
// ストリーミング2D再生
// ストリーミングプレイヤーはプール再利用せず毎回新規作成する。
// BGM は同時に1つしか再生しないことがほとんどのため
// プール管理のコストをかける必要がない。
//-----------------------------------------------------
void SoundManager::PlayStreaming(const std::string& key, bool loop, float startVolume, double startTimeSeconds)
{
    const StreamingSoundData* data = s_loader.GetStreaming(key);
    if (!data || !s_xAudio2) return;

    auto player = std::make_unique<SoundPlayer>(s_xAudio2.Get(), data);
    player->SetVolume(ClampVolume(startVolume));
    player->PlayStreaming(loop, startTimeSeconds);
    s_streamingPlayers[key].push_back(std::move(player));
}

//-----------------------------------------------------
// ストリーミング3D再生
//-----------------------------------------------------
void SoundManager::Play3DStreaming(const std::string& key, const Vector3& position, bool loop, float startVolume, double startTimeSeconds)
{
    const StreamingSoundData* data = s_loader.GetStreaming(key);
    if (!data || !s_xAudio2) return;

    auto player = std::make_unique<SoundPlayer>(s_xAudio2.Get(), data);
    player->SetVolume(ClampVolume(startVolume));
    player->SetPosition(position);
    player->Enable3D(true);
    player->PlayStreaming(loop, startTimeSeconds);
    s_streamingPlayers[key].push_back(std::move(player));
}


void SoundManager::Pause(const std::string& key, bool searchStreaming)
{
    if (SoundPlayer* p = FindActivePlayer(s_players, key))
        p->Pause();

    if (searchStreaming)
    {
        if (SoundPlayer* p = FindActivePlayer(s_streamingPlayers, key))
            p->Pause();
    }
}

void SoundManager::Resume(const std::string& key, bool searchStreaming)
{
    if (SoundPlayer* p = FindActivePlayer(s_players, key))
        p->Resume();

    if (searchStreaming)
    {
        if (SoundPlayer* p = FindActivePlayer(s_streamingPlayers, key))
            p->Resume();
    }
}

//-----------------------------------------------------
// 毎フレーム更新
//-----------------------------------------------------
void SoundManager::Update()
{
    // 通常のサウンドプレイヤーの更新
    for (auto& pair : s_players) 
    {
        auto& vec = pair.second;

        // 3Dオーディオの更新
        for (auto& player : vec)
        {
            if (player && player->IsPlaying() && player->Is3DEnabled()) 
            {
                player->Update3DAudio(s_listener);
            }
        }
 
        // 終了したプレイヤーは一定数だけプールに残し、超過分を削除
        size_t idleKept = 0;
        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [&](const std::unique_ptr<SoundPlayer>& p)
                {
                    if (!p) return true;
                    if (p->IsPlaying()) return false;
                    if (idleKept < kMaxIdlePlayersPerKey)
                    {
                        ++idleKept; // プールとして保持
                        return false;
                    }
                    return true; // 余剰分は破棄
                }),
            vec.end());
    }

    // ストリーミングサウンドプレイヤーの更新
    // Note: UpdateStreaming()は内部で再生状態をチェックし、必要な場合のみバッファを補充します
    for (auto& pair : s_streamingPlayers) 
    {
        auto& vec = pair.second;

        // ストリーミングバッファの更新と3Dオーディオの更新
        for (auto& player : vec)
        {
            if (player->IsPlaying())
            {
                player->UpdateStreaming();
                if (player->Is3DEnabled())
                {
                    player->Update3DAudio(s_listener);
                }
            }
        }

        // 終了したプレイヤーを削除
        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [](const std::unique_ptr<SoundPlayer>& p) { return !p->IsPlaying(); }),
            vec.end()
        );
    }
}

//-----------------------------------------------------
// 全プレイヤーを停止する
//-----------------------------------------------------
void SoundManager::StopAll()
{
    StopAndClearMap(s_players);
    StopAndClearMap(s_streamingPlayers);
}

//-----------------------------------------------------
// 指定キーのサウンドを停止・破棄する
// searchStreaming = true のときストリーミングも対象にする
//-----------------------------------------------------
void SoundManager::Stop(const std::string& key, bool searchStreaming)
{
    auto it = s_players.find(key);
    if (it != s_players.end())
    {
        for (auto& player : it->second)
            if (player) player->Stop();
        s_players.erase(it);
    }

    if (searchStreaming)
    {
        auto its = s_streamingPlayers.find(key);
        if (its != s_streamingPlayers.end())
        {
            for (auto& player : its->second)
                if (player) player->Stop();
            s_streamingPlayers.erase(its);
        }
    }
}


//-----------------------------------------------------
// ボリューム設定
//-----------------------------------------------------
void SoundManager::SetVolume(const std::string& key, float volume)
{
    const float clamped = ClampVolume(volume);
    auto it = s_players.find(key);
    if (it != s_players.end())
    {
        for (auto& player : it->second)
            if (player) player->SetVolume(clamped);
    }
}

//-----------------------------------------------------
// マスターボリュームを変更する
// XAudio2 の MasteringVoice に直接設定するため全音源に即時反映される
//-----------------------------------------------------
void SoundManager::SetMasterVolume(float volume)
{
    if (s_masterVoice)
    {
        s_masterVolume = ClampVolume(volume);
        s_masterVoice->SetVolume(s_masterVolume);
    }
}


//=========================================================
// 再生時間・状態取得
//=========================================================

//-----------------------------------------------------
// アクティブプレイヤーを探して再生済みサンプル数を返す
// BeatManager の BPM 同期計算（OSタイマーより高精度）に使用される
//-----------------------------------------------------
uint64_t SoundManager::GetSamplesPlayed(const std::string& key, bool searchStreaming)
{
    SoundPlayer* player = FindActivePlayer(s_players, key);
    if (!player && searchStreaming)
        player = FindActivePlayer(s_streamingPlayers, key);
    return player ? player->GetSamplesPlayed() : 0ULL;
}

double SoundManager::GetPlaybackTimeSeconds(const std::string& key, bool searchStreaming)
{
    SoundPlayer* player = FindActivePlayer(s_players, key);
    if (!player && searchStreaming)
        player = FindActivePlayer(s_streamingPlayers, key);
    return player ? player->GetPlaybackTimeSeconds() : 0.0;
}

double SoundManager::GetPlaybackTimeMilliseconds(const std::string& key, bool searchStreaming)
{
    return GetPlaybackTimeSeconds(key, searchStreaming) * 1000.0;
}

bool SoundManager::IsSoundActive(const std::string& key, bool searchStreaming)
{
    if (FindActivePlayer(s_players, key)) return true;
    return searchStreaming && FindActivePlayer(s_streamingPlayers, key);
}


//=========================================================
// 3Dオーディオ リスナー設定
//=========================================================

//-----------------------------------------------------
// X3DAUDIO_VECTOR への変換を直接代入で統一する
// 変更前は x/y/z を個別代入していたため行数が3倍になっていた
//-----------------------------------------------------
void SoundManager::SetListenerPosition(const Vector3& position)
{
    s_listener.Position = X3DAUDIO_VECTOR(position.x, position.y, position.z);
}

void SoundManager::SetListenerOrientation(const Vector3& forward, const Vector3& up)
{
    s_listener.OrientFront = X3DAUDIO_VECTOR(forward.x, forward.y, forward.z);
    s_listener.OrientTop   = X3DAUDIO_VECTOR(up.x,      up.y,      up.z);
}

void SoundManager::SetListenerVelocity(const Vector3& velocity)
{
    s_listener.Velocity = X3DAUDIO_VECTOR(velocity.x, velocity.y, velocity.z);
}