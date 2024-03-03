#include <Windows.h>
#include "../../resource/tool.h"
using namespace std;



HINSTANCE hInst;                                // 当前实例



int APIENTRY wUiWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow);
int UiMain(CmdLineW& cl);
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

    }


    if (type == L"main-ui") {
        return UiMain(cl);
    }
    if (type == L"setup") {
        return WndMain_SetupDlg();
    }

    if (cl.argc() < 2) {
        return Process.StartAndWait(L"\"" + GetProgramDirW() + L"\" --type=setup");
    }

    return ERROR_INVALID_PARAMETER;
}



bool InstallProduct(wstring path) {


    return false;
}




