#pragma once
#include "SongScene.h"

class RO : public SongScene
{
public:
    RO()
        : SongScene({
            L"RollOn",              // songTitle
            "BGM_RO",                     // bgmKey
            L"assets\\sound\\BG_RollOn.wav", // bgmPath
            180,                            // bpm
            "assets/score/test.json",        // scorePath
			L"assets\\texture\\C_RO.png" // albumArtPath
            })
    {
    }

protected:
    // リトライ時に自分自身のシーンを再生成
    void OnRetry() override
    {
        Manager::SetScene<RO>();
    }
};