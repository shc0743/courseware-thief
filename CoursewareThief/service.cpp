// service.cpp  
  
#include "service.h"  
  
#include <iostream>
#include <fstream>
#include <string>
#include "../../resource/tool.h"
using namespace std;

DWORD WINAPI srv_main(PVOID);
extern std::wstring szSvcName;
extern HANDLE hServiceWorkerThreads[];

CoursewareThiefService*  CoursewareThiefService::pCurrentService;
  
CoursewareThiefService::CoursewareThiefService(const std::wstring& serviceName)  
    : serviceName(serviceName),  
      stopServiceEvent(INVALID_HANDLE_VALUE),  
      serviceStatusHandle(NULL),
      serviceStatus(),
	  last_stat(0)
{  
    pCurrentService = this;
    szSvcName = serviceName;
}  
  
CoursewareThiefService::~CoursewareThiefService() {  
    if (stopServiceEvent != INVALID_HANDLE_VALUE) {  
        CloseHandle(stopServiceEvent);  
    }
    pCurrentService = nullptr;
}  
  
bool CoursewareThiefService::StartAsService() {  
    PWSTR psvcName = new wchar_t[serviceName.size() + 1];
    wcscpy_s(psvcName, serviceName.size() + 1, serviceName.c_str());
    SERVICE_TABLE_ENTRY serviceTable[] = {  
        { psvcName, (LPSERVICE_MAIN_FUNCTION)CoursewareThiefService::ServiceMain },  
        { NULL, NULL }  
    };  
  
    if (!StartServiceCtrlDispatcher(serviceTable)) {  
        //std::fstream fp("error.log", std::ios::app);
        //fp << "StartServiceCtrlDispatcher failed with error " << GetLastError() << std::endl;  
        return false;  
    }  
  
    delete[] psvcName;
    return true;  
}  
  
void WINAPI CoursewareThiefService::ServiceMain(DWORD argc, LPTSTR* argv) {  
    // Initialize service  
    pCurrentService->serviceStatus.dwServiceType = SERVICE_WIN32;
    pCurrentService->serviceStatus.dwCurrentState = SERVICE_START_PENDING;  
    pCurrentService->serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;  
    pCurrentService->serviceStatus.dwWin32ExitCode = 0;  
    pCurrentService->serviceStatus.dwServiceSpecificExitCode = 0;  
    pCurrentService->serviceStatus.dwCheckPoint = 0;  
    pCurrentService->serviceStatus.dwWaitHint = 0;  
  
    pCurrentService->serviceStatusHandle = RegisterServiceCtrlHandler(argv[0], (LPHANDLER_FUNCTION)CoursewareThiefService::ServiceCtrlHandler);  
  
    if (pCurrentService->serviceStatusHandle == NULL) {
        return;  
    }  
  
    // TODO: Perform service initialization here.  


  
    // Set the service to running state.  
    pCurrentService->serviceStatus.dwCurrentState = SERVICE_RUNNING;  
    SetServiceStatus(pCurrentService->serviceStatusHandle, &pCurrentService->serviceStatus);  
  
    // Create a thread to do the main service work.  
    // TODO: Replace srv_main with your actual service function.  
    HANDLE hThread = CreateThread(NULL, 0, srv_main, NULL, 0, NULL);  
    if (hThread == NULL) {  
        // Handle error  
        pCurrentService->serviceStatus.dwCurrentState = SERVICE_STOPPED;  
        pCurrentService->serviceStatus.dwWin32ExitCode = GetLastError();  
        SetServiceStatus(pCurrentService->serviceStatusHandle, &pCurrentService->serviceStatus);  
        return;  
    }
  
    // Wait until our worker thread exits signaling that the service needs to stop  
    WaitForSingleObject(hThread, INFINITE);  
  
    // Service is stopping  
    pCurrentService->serviceStatus.dwWin32ExitCode = 0;  
    pCurrentService->serviceStatus.dwCurrentState = SERVICE_STOPPED;  
    SetServiceStatus(pCurrentService->serviceStatusHandle, &pCurrentService->serviceStatus);  
}

BOOL CoursewareThiefService::UpdateServiceStatus()
{
	return SetServiceStatus(pCurrentService->serviceStatusHandle,
		&pCurrentService->serviceStatus);
}
  
void WINAPI CoursewareThiefService::ServiceCtrlHandler(DWORD ctrlCode) {  
#if 0
    switch (ctrlCode) {  
  
    case SERVICE_CONTROL_SHUTDOWN:  
        // TODO: Implement shutdown logic here.  
        pCurrentService->serviceStatus.dwWin32ExitCode = 0;  
        pCurrentService->serviceStatus.dwCurrentState = SERVICE_STOPPED;  
        SetServiceStatus(pCurrentService->serviceStatusHandle, &pCurrentService->serviceStatus);  
        return;  

    case SERVICE_CONTROL_STOP:  
        // TODO: Implement stop logic here.  
        pCurrentService->serviceStatus.dwWin32ExitCode = 0;  
        pCurrentService->serviceStatus.dwCurrentState = SERVICE_STOPPED;  
        SetServiceStatus(pCurrentService->serviceStatusHandle, &pCurrentService->serviceStatus);  
        return;  
  
    case SERVICE_CONTROL_PAUSE:  
        pCurrentService->serviceStatus.dwCurrentState = SERVICE_PAUSE_PENDING;  
        SetServiceStatus(pCurrentService->serviceStatusHandle, &pCurrentService->serviceStatus);  

        // TODO: Implement pause logic here.  
        pCurrentService->serviceStatus.dwCurrentState = SERVICE_PAUSED;  
        SetServiceStatus(pCurrentService->serviceStatusHandle, &pCurrentService->serviceStatus);  
        return;  
  
    case SERVICE_CONTROL_CONTINUE:  
        // TODO: Implement continue logic here.  
        pCurrentService->serviceStatus.dwCurrentState = SERVICE_RUNNING;  
        SetServiceStatus(pCurrentService->serviceStatusHandle, &pCurrentService->serviceStatus);  
        return;  
  
    default:  
        break;  
    }  
#else
    switch (ctrlCode)
    {

	case SERVICE_CONTROL_SHUTDOWN: {
		pCurrentService->last_stat = pCurrentService->serviceStatus.dwCurrentState;
		HANDLE hThr = CreateThread(0, 0, StoppingThrd, (PVOID)-1, 0, 0);
		if (hThr) CloseHandle(hThr);
		else ExitProcess(GetLastError());

		pCurrentService->serviceStatus.dwWin32ExitCode = 0;
		pCurrentService->serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		pCurrentService->serviceStatus.dwCheckPoint = 0;
		pCurrentService->serviceStatus.dwWaitHint = 4096;

		break;
	}
	case SERVICE_CONTROL_STOP: {
		pCurrentService->last_stat = pCurrentService->serviceStatus.dwCurrentState;
		HANDLE hThr = CreateThread(0, 0, StoppingThrd, pCurrentService, 0, 0);
		if (hThr) CloseHandle(hThr);
		else ExitProcess(GetLastError());

		pCurrentService->serviceStatus.dwWin32ExitCode = 0;
		pCurrentService->serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		pCurrentService->serviceStatus.dwCheckPoint = 0;
		pCurrentService->serviceStatus.dwWaitHint = 16384;

		break;
	}
	case SERVICE_CONTROL_PAUSE: {
		pCurrentService->last_stat = pCurrentService->serviceStatus.dwCurrentState;
		pCurrentService->serviceStatus.dwWin32ExitCode = 0;
		pCurrentService->serviceStatus.dwCurrentState = SERVICE_PAUSE_PENDING;
		pCurrentService->serviceStatus.dwCheckPoint = 0;
		pCurrentService->serviceStatus.dwWaitHint = 16384;
		UpdateServiceStatus();

		HANDLE hThr = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)
			[](PVOID)->DWORD {
				//Sleep(1000);

				for (size_t i = 0; i < 32; ++i) {
					if (hServiceWorkerThreads[i]) {
						SuspendThread(hServiceWorkerThreads[i]);
					}
				}

				pCurrentService->serviceStatus.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
				pCurrentService->serviceStatus.dwWin32ExitCode = 0;
				pCurrentService->serviceStatus.dwCurrentState = SERVICE_PAUSED;
				pCurrentService->serviceStatus.dwCheckPoint = 0;
				pCurrentService->serviceStatus.dwWaitHint = 0;
				UpdateServiceStatus();
				return 0;
			}, (PVOID)(INT_PTR)2, 0, 0);
		if (hThr) CloseHandle(hThr);
		else {
			pCurrentService->serviceStatus.dwWin32ExitCode = GetLastError();
			pCurrentService->serviceStatus.dwCurrentState = pCurrentService->last_stat;
			pCurrentService->serviceStatus.dwWaitHint = 0;
			//UpdateServiceStatus();
			break;
		}

		pCurrentService->serviceStatus.dwCheckPoint++;
		break;
	}
	case SERVICE_CONTROL_CONTINUE: {
		pCurrentService->serviceStatus.dwWin32ExitCode = 0;
		pCurrentService->serviceStatus.dwCurrentState = SERVICE_CONTINUE_PENDING;
		pCurrentService->serviceStatus.dwCheckPoint = 0;
		pCurrentService->serviceStatus.dwWaitHint = 16384;
		UpdateServiceStatus();

		for (size_t i = 0; i < 32; ++i) {
			if (hServiceWorkerThreads[i]) {
				ResumeThread(hServiceWorkerThreads[i]);
			}
		}

		pCurrentService->serviceStatus.dwControlsAccepted &= ~SERVICE_ACCEPT_STOP;

		pCurrentService->serviceStatus.dwWin32ExitCode = 0;
		pCurrentService->serviceStatus.dwCurrentState = SERVICE_RUNNING;
		pCurrentService->serviceStatus.dwCheckPoint = 0;
		pCurrentService->serviceStatus.dwWaitHint = 0;
		break;
	}
	default:
		return;
    }
#endif
  
    // Report current status  
    SetServiceStatus(pCurrentService->serviceStatusHandle, &pCurrentService->serviceStatus);  
}  



#pragma warning(push)
#pragma warning(disable: 6258)
DWORD __stdcall CoursewareThiefService::StoppingThrd(PVOID arg) {
	pCurrentService->serviceStatus.dwCheckPoint++;
	UpdateServiceStatus();

	if (arg != (PVOID)-1) {
		CoursewareThiefService::End_UI_Process();
	}
	pCurrentService->serviceStatus.dwCheckPoint++;
	UpdateServiceStatus();


	pCurrentService->serviceStatus.dwWin32ExitCode = 0;
	pCurrentService->serviceStatus.dwCurrentState = SERVICE_STOPPED;
	pCurrentService->serviceStatus.dwCheckPoint = 0;
	pCurrentService->serviceStatus.dwWaitHint = 0;
	UpdateServiceStatus();
	return 0;
}

bool GetAllUsersSessionId(std::vector<DWORD>& output) {
    HANDLE hServer = NULL;
    WTS_SESSION_INFO* pSessionInfo = NULL;
    DWORD count = 0;
    bool success = false;

    // 连接到本地终端服务  
    hServer = (WTS_CURRENT_SERVER_HANDLE);

    // 枚举所有会话  
    if (WTSEnumerateSessions(hServer, 0, 1, &pSessionInfo, &count)) {
        // 遍历会话信息并收集会话ID  
        output.clear(); // 清除之前的会话ID（如果有的话）  
        for (DWORD i = 0; i < count; i++) {
            output.push_back(pSessionInfo[i].SessionId);
        }
        success = true;
    }
    else {
        // 枚举会话失败  
        DWORD error = GetLastError();
        // 可以在这里添加错误处理代码  
    }

    // 释放会话信息内存  
    if (pSessionInfo != NULL) {
        WTSFreeMemory(pSessionInfo);
    }

    // 关闭服务器句柄  
    WTSCloseServer(hServer);

    return success;
};

void CoursewareThiefService::End_UI_Process() {
	STARTUPINFO si{ 0 };
	si.cb = sizeof(si);
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW;

	PROCESS_INFORMATION pi{ 0 };

	wstring app_name, s_cmd_line;
	app_name = L"cwtui.exe";
	s_cmd_line = L"cwtui --type=end-user-interface "
		"--service-name=\"" + szSvcName + L"\"";
	WCHAR cmd_line[512]{ 0 };
	wcscpy_s(cmd_line, s_cmd_line.c_str());

	vector<DWORD> sessions;
    GetAllUsersSessionId(sessions);
    for(auto&i:sessions)
        if (CreateProcessInSession(i,
            app_name.c_str(), cmd_line,
            0, 0, 0, 0, 0, 0, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
}
#pragma warning(pop)


  
// TODO: Implement srv_main function in another file. It should have the following signature:  
// DWORD WINAPI srv_main(LPVOID lpParam) {  
//     // Your service logic here.  
//     return 0;  
// }  
  
// You would typically install and start the service using sc.exe or another utility.  
// For example: "sc create MyService binPath= "path_to_your_exe" DisplayName= "My Custom Service""  
// And then: "sc start MyService"





