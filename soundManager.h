#pragma once

#include <wrl/client.h>
#include <xaudio2.h>
#include <X3DAudio.h>
#include <xaudio2fx.h>
#include <map>
#include <memory>
#include <string>
#include <chrono>
#include <cstdint>
#include "soundLoader.h"
#include "soundPlayer.h"
#include "vector3.h"

class SoundManager
{
public:
    SoundManager();
    ~SoundManager();

    // サウンドシステム初期化・終了
    static bool Init();
    static void Uninit();
    static void Update();

    // サウンドのロード //---------------------------------
    // メモリ全体読み込み（短い効果音向け）
    static bool Load(const std::string& key, const std::wstring& filename);
    // ストリーミング読み込み（長いBGM向け）
    static bool LoadStreaming(const std::string& key, const std::wstring& filename);
    //-----------------------------------------------------

    // サウンドの再生（メモリ読み込み音源用）//-----------------------------------
    // ノーマル再生
    static void Play(const std::string& key, bool loop = false, float startVolume = 1.0f, double startTimeSeconds = 0.0);
    // ランダムボリューム・ピッチ付き再生
    static void Play_RandVP(const std::string& key, bool loop, float startVolume = 1.0f,
        float randVolMax = 1.0f, float randVolMin = 1.0f, float randPitMax = 1.0f, float randPitMin = 1.0f, double startTimeSeconds = 0.0);
    // 3D位置ベース再生
    static void Play3D(const std::string& key, const Vector3& position, bool loop = false, float startVolume = 1.0f, double startTimeSeconds = 0.0);
    // 3Dランダムボリューム・ピッチ付き再生
    static void Play3D_RandVP(const std::string& key, const Vector3& position, bool loop = false, float startVolume = 1.0f,
        float randVolMax = 1.0f, float randVolMin = 1.0f, float randPitMax = 1.0f, float randPitMin = 1.0f, double startTimeSeconds = 0.0);
    //------------------------------------------------------

    // ストリーミング再生用 //-------------------------------
    // ストリーミング再生（2D）
    static void PlayStreaming(const std::string& key, bool loop = false, float startVolume = 1.0f, double startTimeSeconds = 0.0);
    // ストリーミング3D位置ベース再生
    static void Play3DStreaming(const std::string& key, const Vector3& position, bool loop = false, float startVolume = 1.0f, double startTimeSeconds = 0.0);
    //ボリューム関連 //-------------------------------------
    static void SetVolume(const std::string& key, float volume = 1.0f);
    static void SetMasterVolume(float volume = 1.0f);
    static float GetMasterVolume() { return s_masterVolume; }

    static uint64_t GetSamplesPlayed(const std::string& key, bool searchStreaming = true);
    static double GetPlaybackTimeSeconds(const std::string& key, bool searchStreaming = true);
    static double GetPlaybackTimeMilliseconds(const std::string& key, bool searchStreaming = true);
    static bool IsSoundActive(const std::string& key, bool searchStreaming = true);
    //------------------------------------------------------

    // 3Dオーディオリスナー設定
    static void SetListenerPosition(const Vector3& position);
    static void SetListenerOrientation(const Vector3& forward, const Vector3& up);
    static void SetListenerVelocity(const Vector3& velocity);

    // 全サウンド停止
    static void StopAll();
    // 指定キーのサウンド停止（ストリーミングも含めるか選択可）
    static void Stop(const std::string& key, bool searchStreaming = true);

    // X3DAudioインスタンス取得
    static X3DAUDIO_HANDLE& GetX3DInstance() { return s_x3dInstance; }

    // 指定キーの一時停止／再開（ストリーミングも選択可）
    static void Pause(const std::string& key, bool searchStreaming = true);
    static void Resume(const std::string& key, bool searchStreaming = true);

private:
    static Microsoft::WRL::ComPtr<IXAudio2> s_xAudio2;
    static IXAudio2MasteringVoice* s_masterVoice;
    static SoundLoader s_loader;
    static std::map<std::string, std::vector<std::unique_ptr<SoundPlayer>>> s_players;
    static std::map<std::string, std::vector<std::unique_ptr<SoundPlayer>>> s_streamingPlayers;
    static float s_masterVolume; // マスターボリューム
    static std::map<std::string, std::chrono::steady_clock::time_point> s_lastPlayTime;

    // 3Dオーディオ関連
    static X3DAUDIO_HANDLE s_x3dInstance;
    static X3DAUDIO_LISTENER s_listener;
    static DWORD s_channelMask;
};
