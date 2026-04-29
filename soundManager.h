//=========================================================
// soundManager.h
// サウンドシステム全体の管理クラス（ファサード）
//
// XAudio2 の初期化・終了、SoundLoader によるリソース管理、
// SoundPlayer によるSE/BGM 再生を一元管理する。
// 呼び出し元は XAudio2 の詳細を知る必要がなく、このクラスのみを使う。
// 全メンバーが static のため、インスタンス化せずに使用する。
//=========================================================
#pragma once
#include "main.h"
#include <xaudio2.h>
#include <X3DAudio.h>
#include <xaudio2fx.h>
#include <chrono>
#include <cstdint>
#include "soundLoader.h"
#include "soundPlayer.h"
#include <optional>

class SoundManager
{
public:
    SoundManager() = delete; // 全メンバーが static のため直接インスタンス化禁止
    ~SoundManager();

    //=========================================================
    // 初期化・終了・更新
    //=========================================================

    // XAudio2・X3DAudio の初期化。二重初期化は内部でガードする
    static bool Init();
    // 全プレイヤーを停止し、XAudio2 と COM を解放する
    static void Uninit();
    // 毎フレーム呼び出し：3D定位更新・バッファ補充・プール管理を行う
    static void Update();

    //=========================================================
    // ロード
    //=========================================================

    // SE 向け：WAV 全体をメモリに展開してキーと紐付ける
    static bool Load(const std::string& key, const std::wstring& filename);
    // BGM 向け：ヘッダ情報のみ読み込み、再生時にファイルから都度読む
    static bool LoadStreaming(const std::string& key, const std::wstring& filename);

    //=========================================================
    // 再生（メモリ音源・SE 向け）
    //=========================================================

    // 通常の2D再生
    static void Play(const std::string& key, bool loop = false, float startVolume = 1.0f, double startTimeSeconds = 0.0);

    // ランダムボリューム・ピッチ付き2D再生
    // 足音や打撃音など同じ音が連続しても単調にならないよう毎回わずかに変動させる
    static void Play_RandVP(const std::string& key, bool loop, float startVolume = 1.0f,
        float randVolMax = 1.0f, float randVolMin = 1.0f, float randPitMax = 1.0f, float randPitMin = 1.0f, double startTimeSeconds = 0.0);

    // 3D空間定位付き再生：リスナーとの距離・方向で音量・定位が変化する
    static void Play3D(const std::string& key, const Vector3& position, bool loop = false, float startVolume = 1.0f, double startTimeSeconds = 0.0);

    // ランダムボリューム・ピッチ付き3D再生
    static void Play3D_RandVP(const std::string& key, const Vector3& position, bool loop = false, float startVolume = 1.0f,
        float randVolMax = 1.0f, float randVolMin = 1.0f, float randPitMax = 1.0f, float randPitMin = 1.0f, double startTimeSeconds = 0.0);

    //=========================================================
    // 再生（ストリーミング・BGM 向け）
    //=========================================================

    // ストリーミング2D再生
    static void PlayStreaming(const std::string& key, bool loop = false, float startVolume = 1.0f, double startTimeSeconds = 0.0);
    // ストリーミング3D再生
    static void Play3DStreaming(const std::string& key, const Vector3& position, bool loop = false, float startVolume = 1.0f, double startTimeSeconds = 0.0);

    //=========================================================
    // 再生コントロール
    //=========================================================

    // 指定キーのサウンドを一時停止（再生位置を保持）
    static void Pause(const std::string& key, bool searchStreaming = true);
    // 指定キーのサウンドを再開
    static void Resume(const std::string& key, bool searchStreaming = true);
    // 指定キーのサウンドを停止・破棄（searchStreaming=true でストリーミングも対象）
    static void Stop(const std::string& key, bool searchStreaming = true);
    // 全サウンドを停止・破棄する（シーン遷移時などに使用）
    static void StopAll();

    //=========================================================
    // ボリューム設定
    //=========================================================

    // 指定キーの全プレイヤーのボリュームを変更する
    static void SetVolume(const std::string& key, float volume = 1.0f);
    // マスターボリュームを変更する（MasteringVoice に即時反映されるため全音源に適用される）
    static void SetMasterVolume(float volume = 1.0f);
    static float GetMasterVolume() { return s_masterVolume; }

    //=========================================================
    // 再生時間・状態取得
    //=========================================================

    // XAudio2 内部サンプル数を返す（OS タイマーより高精度、BPM 同期計算に使用）
    static uint64_t GetSamplesPlayed(const std::string& key, bool searchStreaming = true);
    static double   GetPlaybackTimeSeconds(const std::string& key, bool searchStreaming = true);
    static double   GetPlaybackTimeMilliseconds(const std::string& key, bool searchStreaming = true);
    // 指定キーの音が再生中かどうかを返す
    static bool     IsSoundActive(const std::string& key, bool searchStreaming = true);

    //=========================================================
    // 3Dオーディオ リスナー設定
    //=========================================================

    // リスナー（耳）の位置・向き・速度を設定する
    // 毎フレーム Update() 前に呼んでおくことで正確な3D定位が得られる
    static void SetListenerPosition(const Vector3& position);
    static void SetListenerOrientation(const Vector3& forward, const Vector3& up);
    static void SetListenerVelocity(const Vector3& velocity);

    // SoundPlayer が X3DAudioCalculate を呼ぶために必要なインスタンスを返す
    static X3DAUDIO_HANDLE& GetX3DInstance() { return s_x3dInstance; }

private:
    //=========================================================
    // XAudio2 コアリソース
    //=========================================================
    static Microsoft::WRL::ComPtr<IXAudio2> s_xAudio2;
    static IXAudio2MasteringVoice*          s_masterVoice;  // 全音源の最終出力先
    static SoundLoader                      s_loader;

    //=========================================================
    // プレイヤー管理
    // s_players         : SE 用（プールパターンで再利用）
    // s_streamingPlayers: BGM 用（毎回新規作成・終了後に削除）
    // s_lastPlayTime    : 連打制御（同一キーの最小再生間隔を管理）
    //=========================================================
    static std::unordered_map<std::string, std::vector<std::unique_ptr<SoundPlayer>>> s_players;
    static std::unordered_map<std::string, std::vector<std::unique_ptr<SoundPlayer>>> s_streamingPlayers;
    static std::unordered_map<std::string, std::chrono::steady_clock::time_point>    s_lastPlayTime;

    static float s_masterVolume; // 0.0f〜1.0f にクランプして管理する

    //=========================================================
    // 3Dオーディオ
    //=========================================================
    static X3DAUDIO_HANDLE   s_x3dInstance; // X3DAudioInitialize で生成するハンドル
    static X3DAUDIO_LISTENER s_listener;    // リスナー（耳）の位置・向き・速度
    static DWORD             s_channelMask; // マスターボイスのスピーカー構成（モノ/ステレオ判定）
};
