//=========================================================
// playerInput.cpp
// プレイヤー入力コンポーネント実装
//=========================================================
#include "manager.h"
#include "input.h"
#include "playerInput.h"
#include "soundManager.h"
#include "songScene.h"
#include "judgeSystem.h"

//=========================================================
// 内部ヘルパ・定数
//=========================================================
namespace
{
    // 先行入力の受付時間（秒）
    // この時間以内に入力されたものを判定ウィンドウと照合する
    constexpr double kInputBufferSec = 0.05;

    //-----------------------------------------------------
    // LaneType を配列インデックスに変換する
    // m_inputBuffer の添字として使用するため、enum → int に変換する
    //-----------------------------------------------------
    int LaneToIndex(LaneType lane)
    {
        switch (lane)
        {
        case LaneType::LOW:   return 0;
        case LaneType::MID:   return 1;
        case LaneType::HIGH:  return 2;
        case LaneType::CRASH: return 3;
        default:              return 0;
        }
    }
}


//=========================================================
// ライフサイクル
//=========================================================

void PlayerInput::Init()
{
    // タップ SE をあらかじめロードしておく（再生時の遅延を防ぐ）
    SoundManager::Load("Tap", L"assets\\sound\\SE_Tap.wav");
}


//=========================================================
// 更新
//=========================================================

//-----------------------------------------------------
// 毎フレームの入力処理
//
// 処理の流れ：
//   1. キーボード入力を検出し、入力時刻をバッファに積む（先行入力対応）
//   2. オートプレイ時は JudgeSystem::HandleAutoPlay に委譲して終了
//   3. 通常時はバッファを消費して JudgeSystem::HandleLaneInput で判定する
//      判定に使われなかった入力（空打ち）は小音量でタップ SE を鳴らす
//-----------------------------------------------------
void PlayerInput::Update()
{
    // キーとレーンの対応表（複数キーを同一レーンに割り当て可能）
    struct KeyBind
    {
        std::initializer_list<int> keys;
        LaneType lane;
    };

    static const std::array<KeyBind, 4> keyBinds = { {
        { { 'D' }, LaneType::LOW   },
        { { 'F' }, LaneType::MID   },
        { { 'J' }, LaneType::HIGH  },
        { { 'K' }, LaneType::CRASH }
    } };

    auto* scene = Manager::GetScene();
    if (!scene)
    {
        m_cachedScene     = nullptr;
        m_cachedSongScene = nullptr;
        return;
    }

    // シーンが切り替わったときのみ SongScene へのキャストをやり直す
    if (scene != m_cachedScene)
    {
        m_cachedScene     = scene;
        m_cachedSongScene = dynamic_cast<SongScene*>(scene);
    }

    // タップ SE 再生ラムダ
    // オートプレイ時は SongScene 経由でリクエストし、通常時は直接再生する
    auto playTapSe = [&](float vol)
    {
        if (m_cachedSongScene && m_isAutoPlay)
            m_cachedSongScene->RequestTapSe(vol);
        else
            SoundManager::Play("Tap", false, vol);
    };

    // 現在時刻の取得
    // BeatManager があればそちらを使う（OS タイマーより高精度）
    // なければフォールバックカウンタを使う
    BeatManager* beat = scene->GetGameObject<BeatManager>();
    const double now  = beat ? beat->GetCurrentTimeSeconds() : (m_fallbackTimeSec += FRAME_TIME);

    const auto notesByLane = JudgeSystem::CollectNotesByLane(scene);

    // キー入力を検出して入力バッファに時刻を積む（先行入力）
    for (const auto& bind : keyBinds)
    {
        bool triggered = false;
        for (int key : bind.keys)
        {
            if (Input::GetKeyTrigger(key))
            {
                triggered = true;
                break;
            }
        }
        if (triggered)
            m_inputBuffer[LaneToIndex(bind.lane)].push_back(now);
    }

    // オートプレイ時は JudgeSystem に委譲して終了
    if (m_isAutoPlay)
    {
        Lane* laneObj = scene->GetGameObject<Lane>();
        if (!laneObj) return;
        JudgeSystem::HandleAutoPlay(scene, laneObj, now, notesByLane, playTapSe);
        return;
    }

    Lane* laneObj = scene->GetGameObject<Lane>();
    if (!laneObj) return;

    //-----------------------------------------------------
    // バッファから古い入力を破棄し、有効な入力を1つ消費して判定を試みる
    // kInputBufferSec を超えた入力は期限切れとして捨てる
    //-----------------------------------------------------
    auto consumeBufferedInput = [&](LaneType laneType) -> bool
    {
        auto& q = m_inputBuffer[LaneToIndex(laneType)];
        while (!q.empty() && (now - q.front()) > kInputBufferSec)
            q.pop_front();

        if (!q.empty())
        {
            q.pop_front();
            return true;
        }
        return false;
    };

    constexpr std::array<LaneType, 4> lanes = {
        LaneType::LOW, LaneType::MID, LaneType::HIGH, LaneType::CRASH
    };

    for (auto laneType : lanes)
    {
        if (consumeBufferedInput(laneType))
        {
            const bool judged = JudgeSystem::HandleLaneInput(
                scene, laneObj, laneType, now, notesByLane, playTapSe);

            if (!judged)
            {
                // ノートに対応しない空打ち：小音量でタップ SE を鳴らす
                SoundManager::Play_RandVP("Tap", false, 0.2f, 0.21f, 0.19f);
            }
        }
    }
}