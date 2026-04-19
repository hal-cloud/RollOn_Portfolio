//=========================================================
// texture.h
// テクスチャ管理クラス
//
// WIC 経由で画像ファイルを読み込み、ID3D11ShaderResourceView を生成・キャッシュする。
// 同一ファイルパスへの重複ロードを防ぐため、静的マップでインスタンスを共有する。
// 全メンバーが static のため、インスタンス化せずに使用する。
//=========================================================
#pragma once
#include "main.h"
#include <unordered_map>
#include <string>

class Texture
{
public:
	//-----------------------------------------------------
	// テクスチャのロード
	// 同一パスが既にキャッシュ済みの場合はそのポインタを返す（再生成しない）。
	// 未ロードの場合は WIC でファイルを読み込み、ShaderResourceView を生成してキャッシュする。
	// 戻り値のポインタは s_textureMap が所有するため、呼び出し元は Release() しない。
	//-----------------------------------------------------
	static ID3D11ShaderResourceView* Load(const wchar_t* fileName);

	//-----------------------------------------------------
	// 全テクスチャの解放
	// シーン終了時など、全キャッシュを一括破棄する際に使用する。
	// Release() 後に s_textureMap をクリアし、ダングリングポインタを残さない。
	//-----------------------------------------------------
	static void UnloadAll()
	{
		for (auto& pair : s_textureMap)
		{
			if (pair.second) pair.second->Release();
		}
		s_textureMap.clear();
	}

private:
	// ファイルパスをキーに ShaderResourceView をキャッシュするマップ
	// unordered_map は O(1) 検索のため、毎フレーム呼ばれる Load() のコストを抑えられる
	static std::unordered_map<std::wstring, ID3D11ShaderResourceView*> s_textureMap;
};

