#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#define NOMINMAX
#include <windows.h>
#include <assert.h>
#include <functional>

#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")


#include <DirectXMath.h>
using namespace DirectX;

#include "vector3.h"
#include "json.hpp"
 

#include "DirectXTex.h"
#if _DEBUG
#pragma comment (lib, "DirectXTex_Debug.lib")
#else
#pragma comment (lib, "DirectXTex_Release.lib")
#endif

#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "xaudio2.lib")


#define SCREEN_WIDTH	(1920)
#define SCREEN_HEIGHT	(1080)
#define FRAME_TIME (1.0f / 120.0f)


HWND GetWindow();
