#pragma once
#include "SongScene.h"

class TF : public SongScene
{
public:
    TF()
        : SongScene({
            L"TheFinal",              // songTitle
            "BGM_TF",                     // bgmKey
            L"assets\\sound\\BG_TheFinal.wav", // bgmPath
            220,                            // bpm
            "assets/score/test.json",        // scorePath
			L"assets\\texture\\C_TF.png" // albumArtPath
            })
    {
    }
protected:
    // リトライ時に自分自身のシーンを再生成
    void OnRetry() override
    {
        Manager::SetScene<TF>();
    }
};