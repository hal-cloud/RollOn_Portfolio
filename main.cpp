

#include "main.h"
#include "manager.h"
#include <thread>


const char* g_className = "AppClass";
const char* g_windowName = "RollOn";


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


HWND g_window;

HWND GetWindow()
{
	return g_window;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	WNDCLASSEX wcex;
	{
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = 0;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = nullptr;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = g_className;
		wcex.hIconSm = nullptr;

		RegisterClassEx(&wcex);


		RECT rc = { 0, 0, (LONG)SCREEN_WIDTH, (LONG)SCREEN_HEIGHT };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

		g_window = CreateWindowEx(0, g_className, g_windowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
	}

	CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);


	Manager::Init();

	timeBeginPeriod(1);

	ShowWindow(g_window, nCmdShow);
	UpdateWindow(g_window);

	MSG msg;
	using clock = std::chrono::steady_clock;
	
	const double fixedTimeStep = FRAME_TIME;
	const int maxFrameSkip = 5;

	auto currentTime = clock::now();
	double accumulator = 0.0;

	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			auto newTime = clock::now();
			std::chrono::duration<double> frameTime = newTime - currentTime;
			currentTime = newTime;

			accumulator += frameTime.count();

			int updateCount = 0;
			while (accumulator >= fixedTimeStep && updateCount < maxFrameSkip)
			{
				Manager::Update();
				accumulator -= fixedTimeStep;
				updateCount++;
			}
			Manager::Draw();
		}
	}


	timeEndPeriod(1);

	Manager::Uninit();

	UnregisterClass(g_className, wcex.hInstance);

	CoUninitialize();

	return (int)msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_DELETE:
			DestroyWindow(hWnd);
			break;
		}
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

