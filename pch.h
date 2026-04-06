#pragma once

// === C 標準 ===
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <assert.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <string>

// === Windows / DirectX ===
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#include <DirectXMath.h>
using namespace DirectX;

// === WRL (ComPtr) ===
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

// === サードパーティ ===
#include "DirectXTex.h"
#if _DEBUG
#pragma comment(lib, "DirectXTex_Debug.lib")
#else
#pragma comment(lib, "DirectXTex_Release.lib")
#endif

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "xaudio2.lib")

// === プロジェクト共通 ===
#include "vector3.h"

// === 定数 ===
#define SCREEN_WIDTH  (1920)
#define SCREEN_HEIGHT (1080)
#define FRAME_TIME    (1.0f / 120.0f)

HWND GetWindow();
