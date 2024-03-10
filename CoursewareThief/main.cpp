#include <Windows.h>
#include "../../resource/tool.h"
#include "service.h"
#include "resource.h"
using namespace std;



HINSTANCE hInst;                                // 当前实例



int APIENTRY wUiWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow);
int UiMain(CmdLineW& cl);
int UiMain(CmdLineW& cl, wstring svcName);
int WndMain_SetupDlg();




int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 在此处放置代码。
	::hInst = hInstance;


	CmdLineW cl(GetCommandLineW());
	wstring type; cl.getopt(L"type", type);

	if (type == L"service") {
		SetCurrentDirectoryW((GetProgramDirW() + L"\\..\\").c_str());
		wstring svcName;
		cl.getopt(L"service-name", svcName);
		if (svcName.empty()) return 87;
		CoursewareThiefService ctsvc(svcName);
		return ctsvc.StartAsService() ? 0 : GetLastError();
	}


	if (type == L"main-ui") {
		return UiMain(cl);
	}
	if (type == L"tray") {
		wstring svcname;
		cl.getopt(L"service-name", svcname);
		WCHAR cls[256]{}, title[256]{};
		LoadStringW(hInst, IDS_STRING_WNDCLASS_TRAY, cls, 256);
		HWND hwnd;
		while ((hwnd = FindWindowW(cls, svcname.c_str())))
			PostMessage(hwnd, WM_QUIT, 0, 0);
		return UiMain(cl, svcname);
	}
	if (type == L"open-file") {
		wstring fn; cl.getopt(L"file", fn);
		if (fn.empty()) return 87l;

		WCHAR pipeName[256]{};
		if (!LoadStringW(hInst, IDS_STRING_SERVICE_NAMEDPIPE_NAME,
			pipeName, 256)) return GetLastError();
		HANDLE hPipe = CreateFile(
			pipeName,
			GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, 0, NULL
		);

		if (hPipe == INVALID_HANDLE_VALUE) {
			return GetLastError();
		}

		DWORD bytesWritten;
		WriteFile(hPipe, fn.c_str(), DWORD((fn.length() + 1) * 2), &bytesWritten, NULL);

		wchar_t buffer[2048]{};
		DWORD bytesRead = 0;
		BOOL result = ReadFile(hPipe, buffer, sizeof(buffer) - 2, &bytesRead, NULL);
		if (result && bytesRead > 0) {
			wcout << buffer;
		}
		CloseHandle(hPipe);
		return 0;
	}
	if (type == L"user-shell-open-file") {
		wstring file; cl.getopt(L"file", file);
		if (file.empty()) return 87;
		wstring extName;
		if (file.find(L".") != file.npos) {
			extName = file.substr(file.find_last_of(L"."));
		}
		MessageBox(0, extName.c_str(), 0, 0);
		if (extName == L".pptx" || extName == L".ppt") do{
			wstring originalOpenType, opencmd;
			MyQueryRegistryValue(HKEY_CLASSES_ROOT, extName +
				L"\\CoursewareThiefOriginalValue", originalOpenType);
			if (originalOpenType.empty()) break;
			HKEY hkcmd = NULL;
			RegOpenKeyExW(HKEY_CLASSES_ROOT, (originalOpenType +
				L"\\shell\\open\\command").c_str(), 0, KEY_READ, &hkcmd);
			if (!hkcmd) break;
			MyQueryRegistryValue(hkcmd, L"", opencmd);
			RegCloseKey(hkcmd);
			if (opencmd.empty()) break;
			str_replace(opencmd, L"%1", file);
			return Process.StartOnly(opencmd.c_str()) ? 0 : GetLastError();
		} while (0);
		ShellExecuteW(NULL, L"open", file.c_str(), L"", 0, SW_NORMAL);
		return 0;
	}
	if (type == L"end-user-interface") {
		wstring svcname;
		cl.getopt(L"service-name", svcname);
		WCHAR cls[256]{}, title[256]{};
		LoadStringW(hInst, IDS_STRING_WNDCLASS_TRAY, cls, 256);
		HWND hwnd;
		while ((hwnd = FindWindowW(cls, svcname.c_str())))
			PostMessage(hwnd, WM_QUIT, 0, 0);
		LoadStringW(hInst, IDC_COURSEWARETHIEF, cls, 256);
		LoadStringW(hInst, IDS_APP_TITLE, title, 256);
		while ((hwnd = FindWindowW(cls, NULL)))
			PostMessage(hwnd, WM_CLOSE, 0, 0);
		return 0;
	}
	if (type == L"setup") {
		return WndMain_SetupDlg();
	}

	if (cl.argc() < 2) {
		if (!IsRunAsAdmin()) {
			wstring pw = GetProgramDirW();
			SHELLEXECUTEINFO sei = { 0 };
			sei.cbSize = sizeof(sei);
			sei.fMask = SEE_MASK_NOCLOSEPROCESS;
			sei.hwnd = NULL;
			sei.lpVerb = L"runas"; // 请求管理员权限  
			sei.lpFile = pw.c_str();
			sei.lpParameters = L"--type=setup"; // 程序的参数（如果有的话）  
			sei.lpDirectory = NULL;
			sei.nShow = SW_NORMAL;
			sei.hInstApp = NULL;

			// 执行程序  
			if (!ShellExecuteEx(&sei)) {
				//std::cerr << "ShellExecuteEx failed: " << GetLastError() << std::endl;
				return GetLastError();
			}

			// 等待进程退出  
			if (sei.hProcess != NULL) {
				DWORD code = 0;
				WaitForSingleObject(sei.hProcess, INFINITE); // 无限期等待，直到进程退出  
				GetExitCodeProcess(sei.hProcess, &code);
				CloseHandle(sei.hProcess); // 关闭进程句柄
				return code;
			}
			return ERROR_ACCESS_DENIED;
		}
		return Process.StartAndWait(L"\"" + GetProgramDirW() + L"\" --type=setup");
	}

	return ERROR_INVALID_PARAMETER;
}



constexpr PCWSTR service_name = L"Courseware Thief Service";
bool InstallProduct(wstring path) {
	auto ptype = IsFileOrDirectory(path);
	if (ptype == 1) {
		SetLastError(ERROR_FILE_READ_ONLY);
		return false;
	}
	if (ptype == 0) if (!CreateDirectoryW(path.c_str(), 0)) return false;
	if (!SetCurrentDirectoryW(path.c_str())) return false;
	if (file_exists(L"cwtsrv.exe")) DeleteFileW(L"cwtsrv.exe");
	if (file_exists(L"cwtsrv.exe")) return false;
	if (!CopyFileW(GetProgramDirW().c_str(), L"cwtsrv.exe", FALSE)) return false;

	// create service
	wstring binPath = L"\"" + path + L"/cwtsrv.exe\" --type=service --service-name=\"";
	binPath += service_name;
	binPath += L"\" ";
	wstring svcdesc = L"Create reachable backups for files who is in portable devices "
		"such as an USB-disk on the local machine automatically. If disabled, the "
		"service will stop working.";

	int result = -1;
	do {
		SC_HANDLE sch = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!sch) break;

		DWORD err = 0;
		SC_HANDLE svc = CreateServiceW(sch, service_name, service_name,
			SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,
			SERVICE_ERROR_SEVERE, binPath.c_str(),
			NULL, NULL, L"Power\0LSM\0DcomLaunch\0disk\0", NULL, NULL);
		if (!svc) {
			err = GetLastError();
			CloseServiceHandle(sch);
			SetLastError(err);
			result = err;
			break;
		}

		SERVICE_DESCRIPTIONW a{};
		LPWSTR des2 = (LPWSTR)calloc(sizeof(WCHAR), svcdesc.length() + 1);
		if (des2) {
			wcscpy_s(des2, svcdesc.length() + 1, svcdesc.c_str());
			a.lpDescription = des2;
			ChangeServiceConfig2W(svc, SERVICE_CONFIG_DESCRIPTION, &a);
			free(des2);
		}

		wstring lpc;
		lpc = L"\"" + path + L"/cwtsrv.exe\" --type=service-error-control "
			"--service=\"" + service_name + L"\" --failure --recovery "
			"--failure-count=%1%";
		PWSTR pwc = NULL;
		pwc = (PWSTR)calloc(2, lpc.length() + 1);
		if (pwc) {
			wcscpy_s(pwc, lpc.length() + 1, lpc.c_str());
			SERVICE_FAILURE_ACTIONSW sa{};
			SC_ACTION san[5]{};
			sa.dwResetPeriod = 120;
			sa.lpRebootMsg = NULL;
			sa.lpCommand = pwc;
			sa.cActions = 5;
			sa.lpsaActions = san;
			san[0].Type = SC_ACTION_RESTART;
			san[0].Delay = 1000;
			san[1].Type = SC_ACTION_RESTART;
			san[1].Delay = 2000;
			san[2].Type = SC_ACTION_RESTART;
			san[2].Delay = 5000;
			san[3].Type = SC_ACTION_RUN_COMMAND;
			san[3].Delay = 1;
			san[4].Type = SC_ACTION_NONE;
			san[4].Delay = 0;
			ChangeServiceConfig2W(svc, SERVICE_CONFIG_FAILURE_ACTIONS, &sa);

			free(pwc);
		}

		CloseServiceHandle(svc);
		CloseServiceHandle(sch);
		result = 0;
	} while (0);

	//if (ServiceManager.New(service_name, binPath, ServiceManager.Auto, service_name,
	//    L"Courseware Thief Service (Windows)", SERVICE_WIN32_OWN_PROCESS)) {
	//    DeleteFileW(L"cwtsrv.exe");
	//    return false;
	//}
	ServiceManager.Start(ws2s(service_name));

	return true;
}






