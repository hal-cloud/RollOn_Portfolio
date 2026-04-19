//=========================================================
// player.h
// プレイヤークラス
//
// ゲーム内のプレイヤーを表す GameObject。
// コンポーネントパターンを採用しており、PlayerInput を自身にアタッチして
// 入力処理を委譲する。描画要素は持たず、入力と判定の仲介役となる。
//=========================================================
#pragma once
#include "main.h"
#include "gameObject.h"
#include "playerInput.h"
#include <string>

class Player : public GameObject
{
public:
	void Init();
	void Uninit() override;
	void Update() override;
	void Draw()   override;

	//-----------------------------------------------------
	// オートプレイの ON/OFF を切り替える
	// PlayerInput コンポーネントを検索して ToggleAutoPlay() を呼ぶ
	//-----------------------------------------------------
	void SetAutoPlay();

	//-----------------------------------------------------
	// アタッチするコンポーネントを登録する
	// Init() の冒頭で呼ばれ、PlayerInput を追加する
	//-----------------------------------------------------
	void SetComponents() override
	{
		AddComponent<PlayerInput>();
	}
};