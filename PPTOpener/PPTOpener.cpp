// PPTOpener.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include<windows.h>
#include "../../resource/tool.h"
#include "resource.h"
using namespace std;

#pragma comment(lib, "comctl32.lib") // 链接Common Controls库  

// 窗口过程函数  
LRESULT CALLBACK SplashWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE: {
		// 创建Static控件并加载图片  
		HWND hStatic = CreateWindowEx(0, WC_STATIC, L"", WS_CHILD | WS_VISIBLE | SS_BITMAP,
			0, 0, 0, 0, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

		// 加载BMP资源  
		static HBITMAP bmp = LoadBitmap(GetModuleHandle(0),
			MAKEINTRESOURCE(IDB_BITMAP1));
		// 将图片设置为Static控件的背景  
		SendMessage(hStatic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bmp);

		// 调整窗口大小以匹配图片大小  
		SetWindowPos(hwnd, HWND_TOP, 0, 0, 500, 300, SWP_NOMOVE | SWP_NOZORDER);
			
		break;
	}
	case WM_NCHITTEST:
		return HTCAPTION;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

// 创建Splash窗口的函数  
HWND createSplashWindow(HINSTANCE hInstance) {
	// 初始化Common Controls库  
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

	// 注册窗口类  
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = SplashWndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = L"SplashWindowClass";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	if (!RegisterClass(&wc)) {
		//MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return NULL;
	}

	// 创建窗口  
	HWND hwnd = CreateWindowExW(0, L"SplashWindowClass", L"Splash Screen", WS_POPUP | WS_VISIBLE,
		0, 0, 500, 300, NULL, NULL, hInstance, NULL);
	if (hwnd == NULL) {
		//MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
		return NULL;
	}

	return hwnd;
}

DWORD __stdcall UiThread(PVOID) {
	HWND hwnd = createSplashWindow(ThisInst);
	if (!hwnd) {
		MessageBoxW(NULL, LastErrorStr().c_str(), NULL, MB_ICONERROR);
		return GetLastError();
	}
	CenterWindow(hwnd);
	ShowWindow(hwnd, SW_NORMAL);
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 500, 300, SWP_NOMOVE);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

int _tmain(int argc, wchar_t* argv[])
{
	if (argc < 2) return 87;
	const wchar_t* file = argv[1];
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(file)) {
		MessageBoxW(NULL, LastErrorStrW().c_str(), NULL, MB_ICONERROR);
	}

	HANDLE hUiThread = CreateThread(0, 0, UiThread, 0, 0, 0);
	CloseHandleIfOk(hUiThread);

	Sleep(500);
	ExitProcess(Process.StartAndWait(L"\"" + GetProgramDirW() +
		L"\\..\\cwtsrv.exe\" --type=open-file --file=\"" + file + L"\""));

	return 0;
}


