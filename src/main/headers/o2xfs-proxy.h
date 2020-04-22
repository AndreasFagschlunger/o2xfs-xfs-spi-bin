#ifndef O2XFS_PROXY_H
#define O2XFS_PROXY_H

#include <Windows.h>
#include <tchar.h>

#include <XFSSPI.h>
#include <jni.h>

/*
 * Pointers to the needed JNI invocation API, initialized by LoadJavaVM.
 */
typedef jint (JNICALL *CreateJavaVM_t)(JavaVM **pvm, void **env, void *args);
typedef jint (JNICALL *GetCreatedJavaVMs_t)(JavaVM **vmBuf, jsize bufLen, jsize *nVMs);

typedef struct {
    CreateJavaVM_t CreateJavaVM;
    GetCreatedJavaVMs_t GetCreatedJavaVMs;
} InvocationFunctions;

class Proxy {

public:

	BOOL isInitialized();

	void init(TCHAR *program);

	HRESULT WFPCancelAsyncRequest(HSERVICE hService, REQUESTID RequestID);

	HRESULT WFPClose(HSERVICE hService, HWND hWnd, REQUESTID ReqID);

	HRESULT WFPDeregister(HSERVICE hService, DWORD dwEventClass, HWND hWndReg, HWND hWnd, REQUESTID ReqID);

	HRESULT WFPExecute(HSERVICE hService, DWORD dwCommand, LPVOID lpCmdData, DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID);

	HRESULT WFPGetInfo(HSERVICE hService, DWORD dwCategory, LPVOID lpQueryDetails, DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID);

	HRESULT WFPLock(HSERVICE hService, DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID);

	HRESULT WFPOpen(HSERVICE hService, LPSTR lpszLogicalName, HAPP hApp, LPSTR lpszAppID, DWORD dwTraceLevel, DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID, HPROVIDER hProvider, DWORD dwSPIVersionsRequired, LPWFSVERSION lpSPIVersion, DWORD dwSrvcVersionsRequired, LPWFSVERSION lpSrvcVersion);

	HRESULT WFPRegister(HSERVICE hService, DWORD dwEventClass, HWND hWndReg, HWND hWnd, REQUESTID ReqID);

	HRESULT WFPSetTraceLevel(HSERVICE hService, DWORD dwTraceLevel);

	HRESULT WFPUnloadService();

	HRESULT WFPUnlock(HSERVICE hService, HWND hWnd, REQUESTID ReqID);

};

#endif /* O2XFS_PROXY_H */
