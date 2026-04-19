//=========================================================
// playerInput.h
// プレイヤー入力コンポーネント
//
// Player にアタッチされ、キーボード入力を受け取ってレーン判定へ橋渡しする。
// 入力バッファを持つことで、押したタイミングが判定ウィンドウより
// わずかに早くても拾えるようにしている（先行入力対応）。
// オートプレイモード時は入力を無視し、JudgeSystem が自動で判定を行う。
//=========================================================
#pragma once
#include "component.h"
#include "input.h"
#include <array>
#include <deque>

class Scene;
class SongScene;

class PlayerInput : public Component
{
public:
	void Init();
	void Update();

	// オートプレイの ON/OFF を切り替える（Player::SetAutoPlay() から呼ばれる）
	void ToggleAutoPlay() { m_isAutoPlay = !m_isAutoPlay; }

private:
	bool m_isAutoPlay = false; // true のとき入力を無視して自動判定に切り替わる

	//-----------------------------------------------------
	// 入力バッファ（レーン×4）
	// 入力された時刻を deque で保持し、kInputBufferSec 以内の入力を
	// 判定ウィンドウと照合する。古い入力は Update の冒頭で破棄する。
	//-----------------------------------------------------
	std::array<std::deque<double>, 4> m_inputBuffer{};

	// BeatManager が取得できない場合のフォールバック用経過時間カウンタ
	double m_fallbackTimeSec = 0.0;

	//-----------------------------------------------------
	// シーンキャッシュ
	// 毎フレーム dynamic_cast するコストを避けるため、
	// シーンが変わったタイミングのみキャストし直す
	//-----------------------------------------------------
	Scene*     m_cachedScene     = nullptr;
	SongScene* m_cachedSongScene = nullptr;
};

