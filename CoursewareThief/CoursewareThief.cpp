﻿// CoursewareThief.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "CoursewareThief.h"
#include <uxtheme.h>
#include "../../resource/tool.h"
using namespace std;
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING 256

// 全局变量:
extern HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
WCHAR szTrayClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


#pragma region MyRegion

static LRESULT CALLBACK WndProc_MainWnd(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);
static LRESULT CALLBACK WndProc_TrayWnd(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);

typedef struct {
    HWND hwndRoot;
    HWND
        hList1,
        hBtnExplore, hBtnOk, hBtnCancel;
} WndData_MainWnd, * WndDataP_MainWnd;
typedef struct {
    HWND hwndRoot;
	NOTIFYICONDATAW* pnid;
} WndData_TrayWnd, * WndDataP_TrayWnd;


static HFONT ghFont;
extern std::wstring szSvcName;



int UiMain(CmdLineW& cl) {
	INITCOMMONCONTROLSEX icce{};
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_ALL_CLASSES;
	{void(0); }
	if (!InitCommonControlsEx(&icce)) {
		return GetLastError();
	}

	// 初始化全局字符串
	LoadStringW(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInst, IDC_COURSEWARETHIEF, szWindowClass, MAX_LOADSTRING);

	HICON hIcon = LoadIconW(hInst, MAKEINTRESOURCE(IDI_COURSEWARETHIEF));
	HBRUSH hBg = CreateSolidBrush(RGB(0xF0, 0xF0, 0xF0));
	ghFont = CreateFontW(-14, -7, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FF_DONTCARE,
		L"Consolas");
	s7::MyRegisterClassW(szWindowClass, WndProc_MainWnd, WNDCLASSEXW{
		.hIcon = hIcon,
		.hbrBackground = hBg,
		.lpszMenuName = MAKEINTRESOURCE(IDC_COURSEWARETHIEF),
		.hIconSm = hIcon,
		});

	// 创建窗口  
	WndData_MainWnd wd{};
	HWND hwnd = CreateWindowExW(0, szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		0, 0, 640, 480,
		NULL, NULL, 0, &wd);

	if (hwnd == NULL) {
		return GetLastError();
	}

	// 显示窗口 
	CenterWindow(hwnd);
	ShowWindow(hwnd, SW_NORMAL);

	// 消息循环  
	HACCEL hAccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(IDC_COURSEWARETHIEF));
	MSG msg{};
	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

int UiMain(CmdLineW& cl, wstring svcName) {
	INITCOMMONCONTROLSEX icce{};
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_ALL_CLASSES;
	{void(0); }
	if (!InitCommonControlsEx(&icce)) {
		return GetLastError();
	}

	szSvcName = svcName;

	// 初始化全局字符串
	LoadStringW(hInst, IDS_STRING_WNDCLASS_TRAY, szTrayClass, MAX_LOADSTRING);

	HICON hIcon = LoadIconW(hInst, MAKEINTRESOURCE(IDI_COURSEWARETHIEF));
	ghFont = CreateFontW(-14, -7, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FF_DONTCARE,
		L"Consolas");
	s7::MyRegisterClassW(szTrayClass, WndProc_TrayWnd, WNDCLASSEXW{
		.hIcon = hIcon,
		.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1),
		.hIconSm = hIcon,
		});

	// 创建窗口  
	WndData_MainWnd wd{};
	HWND hwnd = CreateWindowExW(0, szTrayClass,
		svcName.c_str(),
		WS_OVERLAPPED,
		0, 0, 1, 1,
		NULL, NULL, 0, &wd);

	if (hwnd == NULL) {
		return GetLastError();
	}

	 //不显示窗口 
	//CenterWindow(hwnd);
	//ShowWindow(hwnd, SW_NORMAL);

	// 消息循环  
	MSG msg{};
	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}





// 窗口过程函数  
#include <shellapi.h>
static LRESULT CALLBACK WndProc_MainWnd(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
	WndDataP_MainWnd data = (WndDataP_MainWnd)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (message) {
	case WM_CREATE:
	{
		LPCREATESTRUCTW pcr = (LPCREATESTRUCTW)lp;
		if (!pcr) break;
		WndDataP_MainWnd dat = (WndDataP_MainWnd)pcr->lpCreateParams;
		if (!dat) break;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(void*)dat);

		dat->hwndRoot = hwnd;

#define MYCTLS_VAR_HWND hwnd
#define MYCTLS_VAR_HINST NULL
#define MYCTLS_VAR_HFONT ghFont
#include "./ctls.h"

		dat->hList1 = custom(L"", WC_LISTVIEW, 0, 0, 1, 1,
			LVS_REPORT | WS_BORDER);
		dat->hBtnExplore = button(L"文件位置 (&E)", IDABORT);
		dat->hBtnOk = button(L"打开 (&O)", IDOK);
		dat->hBtnCancel = button(L"关闭 (&Q)", IDCANCEL);

		PostMessage(hwnd, WM_USER + 0xf0, 0, 0);


	}
	break;

	case WM_USER + 0xf0:
	{
		if (!data) break;
		SetWindowTheme(data->hList1, L"Explorer", NULL);

		LVCOLUMN lvc{};
		wchar_t sz1[] = L"文件名", sz2[] = L"偷取时间", sz3[] = L"更改时间";

		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;

		lvc.iSubItem = 0;
		lvc.pszText = sz1;
		lvc.cx = 220;               // Width of column in pixels.
		// Insert the columns into the list view.
		ListView_InsertColumn(data->hList1, 0, &lvc);

		lvc.iSubItem = 0;
		lvc.pszText = sz2;
		lvc.cx = 180;               // Width of column in pixels.
		// Insert the columns into the list view.
		ListView_InsertColumn(data->hList1, 1, &lvc);

		lvc.iSubItem = 0;
		lvc.pszText = sz3;
		lvc.cx = 180;               // Width of column in pixels.
		// Insert the columns into the list view.
		ListView_InsertColumn(data->hList1, 2, &lvc);

		DragAcceptFiles(hwnd, TRUE);

		PostMessage(hwnd, WM_USER + 0xf1, 0, 0);
	}
	break;

	case WM_COMMAND:
	{
		// wParam的低位字包含了控件ID，高位字包含了通知代码  
		int id = LOWORD(wp);
		int code = HIWORD(wp);

		// 根据控件ID和通知代码处理不同的消息  
		switch (id) {
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
			break;
		case IDM_EXIT:
		case IDCANCEL:
			SendMessageW(hwnd, WM_CLOSE, 0, 0);
			break;


		
		default:
			// 未知的控件ID，可以调用默认处理或什么都不做  
			break;
		}
		break;
	}
	break;

	case WM_DROPFILES:
	{
		HDROP hDrop = (HDROP)wp;
		wchar_t szFilePath[2048]{};
		int fileCount = DragQueryFile(hDrop, (UINT)-1, NULL, 0); // 获取拖放文件的数量  

		// 遍历所有拖放的文件  
		for (int i = 0; i < fileCount; ++i) {
			DragQueryFile(hDrop, i, szFilePath, 2048); // 获取文件路径

		}

		DragFinish(hDrop);
	}
	break;

	case WM_SIZE:
	{
		if (!data) break;
		static RECT rc{}; GetClientRect(hwnd, &rc);

		SetWindowPos(data->hList1, 0, 10, 10, rc.right - rc.left - 20,
			rc.bottom - rc.top - 60, SWP_NOACTIVATE);
		SetWindowPos(data->hBtnExplore, 0, rc.right - rc.left - 320,
			rc.bottom - rc.top - 40, 130, 30, SWP_NOACTIVATE);
		SetWindowPos(data->hBtnOk, 0, rc.right - rc.left - 180,
			rc.bottom - rc.top - 40, 80, 30, SWP_NOACTIVATE);
		SetWindowPos(data->hBtnCancel, 0, rc.right - rc.left - 90,
			rc.bottom - rc.top - 40, 80, 30, SWP_NOACTIVATE);

	}
	break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, message, wp, lp);
	}

	return 0;
}
static LRESULT CALLBACK WndProc_TrayWnd(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
	WndDataP_TrayWnd data = (WndDataP_TrayWnd)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	static UINT WM_TaskbarCreated = RegisterWindowMessage(TEXT("TaskbarCreated"));
	constexpr UINT MYWM_CREATETRAYICON = WM_USER + 0xf1;
	constexpr UINT MYWM_TRAYICONCALLBACK = WM_USER + 0xf5;
	switch (message) {
	case WM_CREATE:
	{
		LPCREATESTRUCTW pcr = (LPCREATESTRUCTW)lp;
		if (!pcr) break;
		WndDataP_TrayWnd dat = (WndDataP_TrayWnd)pcr->lpCreateParams;
		if (!dat) break;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(void*)dat);

		dat->hwndRoot = hwnd;

		PostMessageW(hwnd, MYWM_CREATETRAYICON, 0, 0);

	}
	break;

	case MYWM_CREATETRAYICON:
	{

		data->pnid = (decltype(data->pnid))calloc(1, sizeof(*data->pnid));
		if (data->pnid == NULL) break;
		data->pnid->cbSize = sizeof(NOTIFYICONDATA);
		data->pnid->hWnd = hwnd;
		data->pnid->uID = 0;
		data->pnid->uFlags = NIF_ICON | NIF_MESSAGE | NIF_INFO | NIF_TIP;
		data->pnid->uCallbackMessage = MYWM_TRAYICONCALLBACK;
		data->pnid->hIcon = LoadIcon(hInst,
			MAKEINTRESOURCEW(IDI_COURSEWARETHIEF));

		//wcscpy_s(data->pnid->szInfo, L"");
		//wcscpy_s(data->pnid->szInfoTitle, L"");
		WCHAR wcs_szTip[256] = { 0 };
		LoadStringW(hInst, IDS_STRING_UI_TASKICONTEXT, wcs_szTip, 256);
		wcscpy_s(data->pnid->szTip, wcs_szTip);

		Shell_NotifyIconW(NIM_ADD, data->pnid);
	}
		break;

	case MYWM_TRAYICONCALLBACK:
		if (lp == WM_LBUTTONUP || lp == WM_RBUTTONUP) {
			POINT pt = { 0 }; int resp = 0;
			GetCursorPos(&pt);
			SetForegroundWindow(hwnd);

			HMENU menu = GetMenu(hwnd);
			HMENU pop = GetSubMenu(menu, 0);

			auto stat = ServiceManager.Query(szSvcName.c_str());
			ModifyMenuW(pop, ID_MENU_servicestatus, 0, ID_MENU_servicestatus,
				(szSvcName + L" 服务" + (stat == ServiceManager.STATUS_START ?
					L"正在运行" : L"未在运行") + L"。").c_str());
			RemoveMenu(pop, ID_MENU_EXIT_UI, 0);
			RemoveMenu(pop, ID_MENU_EXIT_SERVICE, 0);
			RemoveMenu(pop, ID_MENU_UNINSTALL, 0);
			SetMenuDefaultItem(pop, ID_MENU_stolenppts, FALSE);

			TrackPopupMenu(pop, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
			break;
		}
		break;

	case WM_COMMAND:
	{
		// wParam的低位字包含了控件ID，高位字包含了通知代码  
		int id = LOWORD(wp);
		int code = HIWORD(wp);

		// 根据控件ID和通知代码处理不同的消息  
		switch (id) {
		case WM_CLOSE:
			break;

		case ID_MENU_stolenppts:
			Process.StartOnly(L"\"" + GetProgramDirW() + L"\" --type=main-ui"
				" --service-name=\"" + szSvcName + L"\"");
			break;
		
		default:
			// 未知的控件ID，可以调用默认处理或什么都不做  
			break;
		}
		break;
	}
	break;

	case WM_CLOSE:
		break;
	case WM_QUIT:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		if (message == WM_TaskbarCreated) {
			PostMessageW(hwnd, MYWM_CREATETRAYICON, 0, 0);
			break;
		}
		return DefWindowProc(hwnd, message, wp, lp);
	}

	return 0;
}



#pragma endregion




int APIENTRY wUiWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_COURSEWARETHIEF, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_COURSEWARETHIEF));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	DebugBreak();
    return 0;
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	DebugBreak();
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      0, 0, 640, 480, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   CenterWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DebugBreak();
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}



// “安装”框的消息处理程序。
bool InstallProduct(wstring);
UINT64 correct_Setupfile_metadata[] {
	0xc6780661b9f530e7, 0x5c1ba54f72ea25e1,
	0x5a76b74bdd9f1fac,	0x80433211da70451f,
	0x31168dfccc344564, 0x85ba430b7068f45d,
	0x1c4c7c5a6797c3fc, 0xb95ddd71457ae177
};
struct setup_preinstallation_data
{
	UINT64 metadata[8];
	wchar_t path[2048];

};
INT_PTR CALLBACK WndProc_SetupDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
	{
		wstring path;
		if (file_exists(L"D:\\")) path = L"D:\\Courseware Thief Service";
		else {
			wchar_t sys[256]{};
			(void)GetWindowsDirectoryW(sys, 256);
			wcscat_s(sys, L"\\Courseware Thief Service");
		}
		SetDlgItemTextW(hDlg, IDC_EDIT_INSTALL_LOCATION, path.c_str());
	}
        return (INT_PTR)TRUE;

	case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
		if (LOWORD(wParam) == IDYES) {
			HANDLE fp = CreateFileW((GetProgramDirW() + L".pre-installation-data").c_str(),
				GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			if (!fp || fp == INVALID_HANDLE_VALUE) {
				MessageBox(hDlg, LastErrorStr().c_str(), 0, MB_ICONHAND);
				return 0;
			}

			setup_preinstallation_data spd{};
			for (size_t i = 0; i < 8; ++i) {
				spd.metadata[i] = correct_Setupfile_metadata[i];
			}
			GetDlgItemTextW(hDlg, IDC_EDIT_INSTALL_LOCATION, spd.path, 2048);
			
			DWORD dwW = 0;
			WriteFile(fp, &spd, sizeof(spd), &dwW, 0);
			CloseHandle(fp);
			MessageBoxTimeoutW(0, L"预安装已完成。", L"Success",
				MB_ICONINFORMATION, 0, 30000);

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDOK) {
			WCHAR path[2048]{};
			GetDlgItemTextW(hDlg, IDC_EDIT_INSTALL_LOCATION, path, 2048);
			bool bSuccess = InstallProduct(path);
			MessageBoxTimeoutW(0, LastErrorStrW().c_str(), bSuccess ?
				L"Success" : 0, bSuccess ? MB_ICONINFORMATION : MB_ICONERROR,
				0, 30000);
			if (bSuccess) {
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
		}
        break;
    }
    return (INT_PTR)FALSE;
}
int WndMain_SetupDlg() {
	HANDLE fp = CreateFileW((GetProgramDirW() + L".pre-installation-data").c_str(),
		GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (fp && fp != INVALID_HANDLE_VALUE) {
		setup_preinstallation_data spd{};
		DWORD dwRead = 0;
		if (ReadFile(fp, &spd, sizeof(spd), &dwRead, 0) && dwRead) {
			bool ok = true;
			for (size_t i = 0; i < 8; ++i) {
				if (spd.metadata[i] != correct_Setupfile_metadata[i]) {
					ok = false; break;
				}
			}

			if (ok) {
				bool bSuccess = InstallProduct(spd.path);
				MessageBoxTimeoutW(0, LastErrorStrW().c_str(), bSuccess ?
					L"Success" : 0, bSuccess ? MB_ICONINFORMATION : MB_ICONERROR,
					0, 30000);

				if (bSuccess) return 0;

			}
		}

		CloseHandle(fp);
	}
	return (int)DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_SETUPDLG),
		NULL, WndProc_SetupDlg);
}


