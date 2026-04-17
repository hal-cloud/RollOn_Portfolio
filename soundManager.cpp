#include "soundManager.h"
#include <random>
#include <algorithm>

namespace
{
	constexpr size_t kMaxIdlePlayersPerKey = 8;     // 各キーごとに再利用のために保持する再生終了プレイヤーの最大数
    constexpr long long kMinPlayIntervalMs = 30;    // 同一キーの最小再生間隔

	// キーごとに複数のプレイヤーを管理するためのマップ
    using PlayerMap = std::map<std::string, std::vector<std::unique_ptr<SoundPlayer>>>;

	//-------------------------------
	// 指定キーの再生中プレイヤーを検索
    //-------------------------------
    SoundPlayer* FindActivePlayer(PlayerMap& players, const std::string& key)
    {
        auto it = players.find(key);
        if (it == players.end())
        {
            return nullptr;
        }

        for (auto& player : it->second)
        {
            if (player && player->IsPlaying())
            {
                return player.get();
            }
        }
        return nullptr;
    }

    //---------------------------------
    // 再生終了済みのプレイヤーを再利用し、なければ新規作成
    //---------------------------------
    SoundPlayer* AcquireReusablePlayer(std::vector<std::unique_ptr<SoundPlayer>>& list,
                                       const SoundData* data,
                                       IXAudio2* xAudio2)
    {
        for (auto& p : list)
        {
            if (p && !p->IsPlaying())
            {
                return p.get();
            }
        }

        list.push_back(std::make_unique<SoundPlayer>(xAudio2, data));
        return list.back().get();
    }

    //---------------------------------
	// 最小再生間隔チェックヘルパ
    //---------------------------------
    bool CheckAndUpdatePlayInterval(
        std::map<std::string, std::chrono::steady_clock::time_point>& lastPlayTime,
        const std::string& key, bool loop)
    {
        if (loop) return true;
        auto now = std::chrono::steady_clock::now();
        auto it = lastPlayTime.find(key);
        if (it != lastPlayTime.end())
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
            if (elapsed.count() < kMinPlayIntervalMs) return false;
        }
        lastPlayTime[key] = now;
        return true;
    }
}

//=====================================
// 静的メンバーの定義
//=====================================
Microsoft::WRL::ComPtr<IXAudio2> SoundManager::s_xAudio2;
IXAudio2MasteringVoice* SoundManager::s_masterVoice = nullptr;
SoundLoader SoundManager::s_loader;
std::map<std::string, std::vector<std::unique_ptr<SoundPlayer>>> SoundManager::s_players;
std::map<std::string, std::vector<std::unique_ptr<SoundPlayer>>> SoundManager::s_streamingPlayers;
float SoundManager::s_masterVolume = 1.0f;
std::map<std::string, std::chrono::steady_clock::time_point> SoundManager::s_lastPlayTime;

// 3Dオーディオ関連の静的メンバー初期化
X3DAUDIO_HANDLE SoundManager::s_x3dInstance = {};
X3DAUDIO_LISTENER SoundManager::s_listener = {};
DWORD SoundManager::s_channelMask = 0;


SoundManager::SoundManager()
{
}

SoundManager::~SoundManager() 
{
    Uninit();
}

bool SoundManager::Init()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(XAudio2Create(&s_xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) return false;
    if (FAILED(s_xAudio2->CreateMasteringVoice(&s_masterVoice))) return false;

    // X3DAudioの初期化
    XAUDIO2_VOICE_DETAILS details;
    s_masterVoice->GetVoiceDetails(&details);
    s_channelMask = details.InputChannels == 1 ? SPEAKER_MONO : SPEAKER_STEREO;

    if (FAILED(X3DAudioInitialize(s_channelMask, X3DAUDIO_SPEED_OF_SOUND, s_x3dInstance))) {
        return false;
    }

    // リスナーの初期化
    s_listener.Position = X3DAUDIO_VECTOR(0.0f, 0.0f, 0.0f);
    s_listener.OrientFront = X3DAUDIO_VECTOR(0.0f, 0.0f, 1.0f);
    s_listener.OrientTop = X3DAUDIO_VECTOR(0.0f, 1.0f, 0.0f);
    s_listener.Velocity = X3DAUDIO_VECTOR(0.0f, 0.0f, 0.0f);

    return true;
}

void SoundManager::Uninit() 
{
    StopAll();
    s_players.clear();
    s_loader.Clear();
    if (s_masterVoice) {
        s_masterVoice->DestroyVoice();
        s_masterVoice = nullptr;
    }
    s_xAudio2.Reset();
    CoUninitialize();
}

bool SoundManager::Load(const std::string& key, const std::wstring& filename)
{
    return s_loader.Load(key, filename);
}

bool SoundManager::LoadStreaming(const std::string& key, const std::wstring& filename)
{
    return s_loader.LoadStreaming(key, filename);
}

//------------------------------------
// ノーマル再生
//------------------------------------
void SoundManager::Play(const std::string& key, bool loop, float startVolume, double startTimeSeconds)
{
    const SoundData* data = s_loader.Get(key);
    if (!data || !s_xAudio2) return;

	if (!CheckAndUpdatePlayInterval(s_lastPlayTime, key, loop)) return;

    auto& playerList = s_players[key];
    SoundPlayer* player = AcquireReusablePlayer(playerList, data, s_xAudio2.Get());
    if (!player) return;

    player->SetVolume(startVolume);
    player->Play(loop, startTimeSeconds);
}

//-------------------------------------
// ランダムボリューム・ピッチ付き再生
//-------------------------------------
void SoundManager::Play_RandVP(const std::string& key, bool loop, float startVolume,
    float randVolMax, float randVolMin, float randPitMax, float randPitMin, double startTimeSeconds)
{
    const SoundData* data = s_loader.Get(key);
    if (!data || !s_xAudio2) return;

    if (!CheckAndUpdatePlayInterval(s_lastPlayTime, key, loop)) return;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> volDist(randVolMin, randVolMax);
    std::uniform_real_distribution<float> pitDist(randPitMin, randPitMax);

    auto& playerList = s_players[key];
    SoundPlayer* player = AcquireReusablePlayer(playerList, data, s_xAudio2.Get());
    if (!player) return;

    player->SetVolume(startVolume * volDist(gen));
    player->SetPitch(pitDist(gen));
    player->Play(loop, startTimeSeconds);
}

//-------------------------------------
// 3D位置ベース再生
//-------------------------------------
void SoundManager::Play3D(const std::string& key, const Vector3& position, bool loop, float startVolume, double startTimeSeconds)
{
    const SoundData* data = s_loader.Get(key);
    if (!data || !s_xAudio2) return;

    auto& playerList = s_players[key];
    SoundPlayer* player = AcquireReusablePlayer(playerList, data, s_xAudio2.Get());
    if (!player) return;

    player->SetVolume(startVolume);
    player->SetPosition(position);
    player->Enable3D(true);
    player->Play(loop, startTimeSeconds);
}


void SoundManager::Play3D_RandVP(const std::string& key, const Vector3& position, bool loop, float startVolume,
    float randVolMax, float randVolMin, float randPitMax, float randPitMin, double startTimeSeconds)
{
    const SoundData* data = s_loader.Get(key);
    if (!data || !s_xAudio2) return;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> volDist(randVolMin, randVolMax);
    std::uniform_real_distribution<float> pitDist(randPitMin, randPitMax);


    auto& playerList = s_players[key];
    SoundPlayer* player = AcquireReusablePlayer(playerList, data, s_xAudio2.Get());
    if (!player) return;

    player->SetVolume(startVolume * volDist(gen));
	player->SetPitch(pitDist(gen));
    player->SetPosition(position);
    player->Enable3D(true);
    player->Play(loop, startTimeSeconds);
}

// 再生終了したプレイヤーのクリーンアップと3D更新
void SoundManager::Update()
{
    // 通常のサウンドプレイヤーの更新
    for (auto& pair : s_players) {
        auto& vec = pair.second;

        // 3Dオーディオの更新
        for (auto& player : vec) {
            if (player->IsPlaying() && player->Is3DEnabled()) {
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
    for (auto& pair : s_streamingPlayers) {
        auto& vec = pair.second;

        // ストリーミングバッファの更新と3Dオーディオの更新
        for (auto& player : vec) {
            if (player->IsPlaying()) {
                player->UpdateStreaming();
                if (player->Is3DEnabled()) {
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

void SoundManager::SetVolume(const std::string& key, float volume)
{
    auto it = s_players.find(key);
    if (it != s_players.end()) {
        for (auto& player : it->second) {
            player->SetVolume(volume);
        }
    }
}

void SoundManager::SetMasterVolume(float volume)
{
    if (s_masterVoice) {
        s_masterVolume = volume; // マスターボリューム更新
        s_masterVoice->SetVolume(s_masterVolume);
    }
}

uint64_t SoundManager::GetSamplesPlayed(const std::string& key, bool searchStreaming)
{
    SoundPlayer* player = FindActivePlayer(s_players, key);
    if (!player && searchStreaming)
    {
        player = FindActivePlayer(s_streamingPlayers, key);
    }
    return player ? player->GetSamplesPlayed() : 0ULL;
}

double SoundManager::GetPlaybackTimeSeconds(const std::string& key, bool searchStreaming)
{
    SoundPlayer* player = FindActivePlayer(s_players, key);
    if (!player && searchStreaming)
    {
        player = FindActivePlayer(s_streamingPlayers, key);
    }
    return player ? player->GetPlaybackTimeSeconds() : 0.0;
}

double SoundManager::GetPlaybackTimeMilliseconds(const std::string& key, bool searchStreaming)
{
    return GetPlaybackTimeSeconds(key, searchStreaming) * 1000.0;
}

bool SoundManager::IsSoundActive(const std::string& key, bool searchStreaming)
{
    if (FindActivePlayer(s_players, key))
    {
        return true;
    }

    return searchStreaming && FindActivePlayer(s_streamingPlayers, key);
}

//-------------------------------------
// 3Dオーディオリスナー設定
//-------------------------------------
void SoundManager::SetListenerPosition(const Vector3& position)
{
    s_listener.Position.x = position.x;
    s_listener.Position.y = position.y;
    s_listener.Position.z = position.z;
}

void SoundManager::SetListenerOrientation(const Vector3& forward, const Vector3& up)
{
    s_listener.OrientFront.x = forward.x;
    s_listener.OrientFront.y = forward.y;
    s_listener.OrientFront.z = forward.z;
    
    s_listener.OrientTop.x = up.x;
    s_listener.OrientTop.y = up.y;
    s_listener.OrientTop.z = up.z;
}

void SoundManager::SetListenerVelocity(const Vector3& velocity)
{
    s_listener.Velocity.x = velocity.x;
    s_listener.Velocity.y = velocity.y;
    s_listener.Velocity.z = velocity.z;
}

void SoundManager::StopAll()
{
    for (auto& pair : s_players) {
        for (auto& player : pair.second) {
            player->Stop();
        }
    }
    s_players.clear();

    for (auto& pair : s_streamingPlayers) {
        for (auto& player : pair.second) {
            player->Stop();
        }
    }
    s_streamingPlayers.clear();
}

void SoundManager::Stop(const std::string& key, bool searchStreaming)
{
    auto it = s_players.find(key);
    if (it != s_players.end()) {
        for (auto& player : it->second) {
            player->Stop();
        }
        s_players.erase(it);
    }

    if (searchStreaming) {
        auto its = s_streamingPlayers.find(key);
        if (its != s_streamingPlayers.end()) {
            for (auto& player : its->second) {
                player->Stop();
            }
            s_streamingPlayers.erase(its);
        }
    }
}

//------------------------------------
// ストリーミング再生（2D）
//------------------------------------
void SoundManager::PlayStreaming(const std::string& key, bool loop, float startVolume, double startTimeSeconds)
{
    const StreamingSoundData* data = s_loader.GetStreaming(key);
    if (!data || !s_xAudio2) return;

    auto& playerList = s_streamingPlayers[key];
    auto player = std::make_unique<SoundPlayer>(s_xAudio2.Get(), data);

    player->SetVolume(startVolume);
    player->PlayStreaming(loop, startTimeSeconds);
    playerList.push_back(std::move(player));
}

//-------------------------------------
// ストリーミング3D位置ベース再生
//-------------------------------------
void SoundManager::Play3DStreaming(const std::string& key, const Vector3& position, bool loop, float startVolume, double startTimeSeconds)
{
    const StreamingSoundData* data = s_loader.GetStreaming(key);
    if (!data || !s_xAudio2) return;

    auto& playerList = s_streamingPlayers[key];
    auto player = std::make_unique<SoundPlayer>(s_xAudio2.Get(), data);

    player->SetVolume(startVolume);
    player->SetPosition(position);
    player->Enable3D(true);
    player->PlayStreaming(loop, startTimeSeconds);
    playerList.push_back(std::move(player));
}

void SoundManager::Pause(const std::string& key, bool searchStreaming)
{
    if (SoundPlayer* p = FindActivePlayer(s_players, key))
    {
        p->Pause();
    }
    if (searchStreaming)
    {
        if (SoundPlayer* p = FindActivePlayer(s_streamingPlayers, key))
        {
        p->Pause();
        }
    }
}

void SoundManager::Resume(const std::string& key, bool searchStreaming)
{
    if (SoundPlayer* p = FindActivePlayer(s_players, key))
    {
        p->Resume();
    }
    if (searchStreaming)
    {
        if (SoundPlayer* p = FindActivePlayer(s_streamingPlayers, key))
        {
            p->Resume();
        }
    }
}