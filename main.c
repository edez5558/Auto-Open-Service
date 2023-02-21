#include <windows.h>
#include <tlhelp32.h>

#define SERVICE_NAME TEXT("MonitorAPPService")
#define EXECUTE_PATH TEXT("path/execute.exe")
#define EXECUTE_NAME TEXT("execute.exe")

#define TIME_CHECK_SECONDS 15

SERVICE_STATUS_HANDLE   gSvcStatusHandle;
SERVICE_STATUS          gSvcStatus;

void WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv);
void WINAPI SvcCtrlHandler(DWORD dwCtrl);

int findProcess(const wchar_t* name){
    HANDLE hProcessSnap;
    HANDLE hProcess;
    int pid = -1;
    PROCESSENTRY32 pe32;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if(hProcessSnap == INVALID_HANDLE_VALUE)
        return -2;

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if(!Process32First(hProcessSnap,&pe32)){
        CloseHandle(hProcessSnap);
        return -2;
    }

    do{
        if(wcscmp(name,(const wchar_t*)pe32.szExeFile) == 0){
            pid = pe32.th32ProcessID;
            break;
        }
    }while(Process32Next(hProcess,&pe32));

    CloseHandle(hProcessSnap);

    return pid;
}

void createProcess(const char* path_execute){
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si,sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi,sizeof(pi));

    if(!CreateProcess(NULL,
        (LPSTR)path_execute,
        NULL,NULL,FALSE,
        0,NULL,NULL,
        &si,&pi
    ))
        return;

    WaitForSingleObject(pi.hProcess,INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

int main(){
    SERVICE_TABLE_ENTRY DispatchTable[] = {
        { SERVICE_NAME, SvcMain},
        { NULL, NULL}
    };

    StartServiceCtrlDispatcher(DispatchTable);

    return 0;
}

void SvcMain(DWORD dwArgc, LPTSTR* lpszArgv){
    gSvcStatusHandle = RegisterServiceCtrlHandler(
        SERVICE_NAME,SvcCtrlHandler
    );

    if(!gSvcStatusHandle)
        return;
    
    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    gSvcStatus.dwServiceSpecificExitCode = 0;
    gSvcStatus.dwWin32ExitCode = 0;
    gSvcStatus.dwCheckPoint = 0;
    gSvcStatus.dwWaitHint = 0;

    gSvcStatus.dwCurrentState = SERVICE_RUNNING;

    SetServiceStatus(gSvcStatusHandle,&gSvcStatus);

    while(gSvcStatus.dwCurrentState == SERVICE_RUNNING){
        if(findProcess(EXECUTE_NAME) == -1){
            createProcess(EXECUTE_PATH);
        }

        Sleep(TIME_CHECK_SECONDS * 1000);
    }
}

void SvcCtrlHandler(DWORD dwCtrl){
    switch(dwCtrl){
        case SERVICE_CONTROL_STOP:
            gSvcStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(gSvcStatusHandle,&gSvcStatus);
            break;
        case SERVICE_CONTROL_SHUTDOWN:
            gSvcStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(gSvcStatusHandle,&gSvcStatus);
            break;
    }
}
