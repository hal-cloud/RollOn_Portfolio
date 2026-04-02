#pragma once
#include "main.h"
#include <unordered_map>
#include <string>

class Texture
{
private:
	static std::unordered_map<std::wstring, ID3D11ShaderResourceView*> s_textureMap;
public:
	static ID3D11ShaderResourceView* Load(const wchar_t* fileName);
	static void UnloadAll()
	{
		for (auto& pair : s_textureMap)
		{
			if (pair.second) pair.second->Release();
		}
		s_textureMap.clear();
	}
	
};

