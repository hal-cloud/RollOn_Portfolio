#include "songScene.h"
#include "manager.h"
#include "game.h"

void SongScene::QuitToGame()
{
    SoundManager::StopAll();
    SoundManager::SetMasterVolume(m_masterVolume);
    Manager::SetScene<Game>(); // TitleƒV[ƒ“‚Ö‘JˆÚ
}