#pragma once
#include "SongScene.h"

class OtNR : public SongScene
{
public:
    OtNR()
        : SongScene({
            L"OntheNightRoad",              // songTitle
            "BGM_OtNR",                     // bgmKey
            L"assets\\sound\\BG_OntheNightRoad.wav", // bgmPath
            160,                            // bpm
            "assets/score/test.json",        // scorePath
			L"assets\\texture\\C_OtNR.png" // albumArtPath
            })
    {
    }
protected:
    // リトライ時に自分自身のシーンを再生成
    void OnRetry() override
    {
        Manager::SetScene<OtNR>();
    }
};