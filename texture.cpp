//=========================================================
// texture.cpp
// テクスチャ管理クラス実装
//=========================================================
#include "texture.h"
#include "renderer.h"

//=========================================================
// 静的メンバーの定義
//=========================================================

// ファイルパス → ShaderResourceView のキャッシュマップ
std::unordered_map<std::wstring, ComPtr<ID3D11ShaderResourceView>> Texture::s_textureMap;


//=========================================================
// テクスチャのロード
//=========================================================

//-----------------------------------------------------
// 指定パスのテクスチャを読み込み ShaderResourceView を返す
//
// 既にキャッシュ済みの場合は生成をスキップして既存のポインタを返す。
// 未ロードの場合は以下の手順で生成する：
//   1. LoadFromWICFile でファイルを読み込み ScratchImage に展開
//   2. CreateShaderResourceView で GPU リソースを生成
//   3. s_textureMap にキャッシュしてポインタを返す
//
// assert でロード失敗を検出する。ファイルパスや形式を確認すること。
//-----------------------------------------------------
ID3D11ShaderResourceView* Texture::Load(const wchar_t* fileName)
{
	auto it = s_textureMap.find(fileName);
	if (it != s_textureMap.end()) return it->second.Get();

	// 未ロードの場合は WIC 経由でファイルを読み込み ShaderResourceView を生成する
	ID3D11ShaderResourceView* texture;
	TexMetadata  metadata;
	ScratchImage image;

	LoadFromWICFile(fileName, WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &texture);
	assert(texture); // ロード失敗時はファイルパス・フォーマットを確認すること

	// 次回以降の重複ロードを防ぐためキャッシュに登録
	s_textureMap[fileName] = texture;
	return texture;
}

