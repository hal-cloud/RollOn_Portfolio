#pragma once
#include "SongScene.h"

class Tutorial : public SongScene
{
public:
    Tutorial()
        : SongScene({
            L"Tutorial",              // songTitle
            "BGM_Tutorial",                     // bgmKey
            L"assets\\sound\\BG_Tutorial.wav", // bgmPath
            130,                            // bpm
            "assets/score/test.json",        // scorePath
			L"assets\\texture\\C_Tutorial.png" // albumArtPath
            })
    {
    }
protected:
    // リトライ時に自分自身のシーンを再生成
    void OnRetry() override
    {
        Manager::SetScene<Tutorial>();
    }
};