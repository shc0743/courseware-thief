#pragma once
// service.h  

#ifndef SERVICE_H  
#define SERVICE_H  

#include <windows.h>  
#include <string>  
#include <thread>  

class CoursewareThiefService {
public:
    CoursewareThiefService(const std::wstring& serviceName);
    virtual ~CoursewareThiefService();

    bool StartAsService();
    static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
    static void WINAPI ServiceCtrlHandler(DWORD ctrlCode);

    // TODO: Declare additional methods and members as needed for your service logic.  

    static CoursewareThiefService* pCurrentService;

protected:
    SERVICE_STATUS_HANDLE serviceStatusHandle;
    SERVICE_STATUS serviceStatus;
    HANDLE stopServiceEvent;
    std::wstring serviceName;
    DWORD last_stat;


    static DWORD WINAPI StoppingThrd(PVOID);
    static void End_UI_Process();
    static BOOL UpdateServiceStatus();


    // Disallow copying and assignment.  
    CoursewareThiefService(const CoursewareThiefService&) = delete;
    CoursewareThiefService& operator=(const CoursewareThiefService&) = delete;
};

#endif // SERVICE_H