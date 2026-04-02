#include "texture.h"
#include "renderer.h"

std::unordered_map<std::wstring, ID3D11ShaderResourceView*> Texture::s_textureMap;

ID3D11ShaderResourceView* Texture::Load(const wchar_t* fileName)
{
	if (s_textureMap.count(fileName) > 0)
	{
		return s_textureMap[fileName];
	};
	// テクスチャが読み込まれていない場合は読み込み
	ID3D11ShaderResourceView* texture;
	TexMetadata metadata;
	ScratchImage image;

	LoadFromWICFile((wchar_t*)fileName, WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &texture);
	assert(texture);
	s_textureMap[fileName] = texture;
	return texture;
}

