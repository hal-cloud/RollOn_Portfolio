//=========================================================
// component.h
// ゲームオブジェクト用コンポーネント基底クラス
//
// GameObject にアタッチして機能を追加するコンポーネントパターンの基底。
// 各コンポーネントは m_gameObject を通じて親オブジェクトにアクセスできる。
// 全ての仮想関数はデフォルトで空実装のため、
// 派生クラスは必要なメソッドだけをオーバーライドすればよい。
//=========================================================
#pragma once

class GameObject;

class Component
{
public:
	Component() {}
	// 親オブジェクトを明示して生成するコンストラクタ
	Component(GameObject* Object) { m_gameObject = Object; }

	virtual ~Component() {}

	//-----------------------------------------------------
	// ライフサイクル
	// GameObject の Init/Uninit/Update/Draw から順に呼ばれる。
	// 全てデフォルト空実装のため、不要なメソッドはオーバーライド不要。
	//-----------------------------------------------------
	virtual void Init()   {}
	virtual void Uninit() {}
	virtual void Update() {}
	virtual void Draw()   {}

protected:
	// アタッチ先の GameObject（所有権は GameObject 側が持つ）
	GameObject* m_gameObject = nullptr;
};