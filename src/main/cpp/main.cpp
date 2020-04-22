#include <Windows.h>
#include <XFSSPI.h>

#include "o2xfs-logger.h"
#include "o2xfs-proxy.h"

static TCHAR program[MAX_PATH];
static Proxy PROXY;

o2xfs::Logger LOG;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		LOG.setFileName(TEXT("C:\\Temp\\o2xfs-spi.log"));
		LOG.info(TEXT("DLL_PROCESS_ATTACH"));
		if (GetModuleFileName(hinstDLL, program, MAX_PATH) == 0) {
			LOG.error(TEXT("GetModuleFileName: %d"), GetLastError());
		}
		break;
	case DLL_THREAD_ATTACH:
		LOG.info(_T("DLL_THREAD_ATTACH: %d"), GetCurrentThreadId());
		break;
	case DLL_THREAD_DETACH:
		LOG.info(_T("DLL_THREAD_DETACH: %d"), GetCurrentThreadId());
		break;
	}
	return TRUE;
}

Proxy GetProxy() {
	if (!PROXY.isInitialized()) {
		PROXY.init(program);
	}
	return PROXY;
}

HRESULT extern WINAPI WFPCancelAsyncRequest(HSERVICE hService, REQUESTID ReqID) {
	LOG.info(_T("WFPCancelAsyncRequest: hService=%d,ReqID=%lu"), hService, ReqID);
	return GetProxy().WFPCancelAsyncRequest(hService, ReqID);
}

HRESULT extern WINAPI WFPClose(HSERVICE hService, HWND hWnd, REQUESTID ReqID) {
	LOG.info(_T("WFPClose: hService=%d,hWnd=%p,ReqID=%lu"), hService, hWnd, ReqID);
	return GetProxy().WFPClose(hService, hWnd, ReqID);
}

HRESULT extern WINAPI WFPDeregister(HSERVICE hService, DWORD dwEventClass, HWND hWndReg, HWND hWnd, REQUESTID ReqID) {
	LOG.info(_T("WFPDeregister: hService=%d,dwEventClass=%d,hWndReg=%d,hWnd=%p,ReqID=%lu"), hService, dwEventClass, hWndReg, hWnd, ReqID);
	return GetProxy().WFPDeregister(hService, dwEventClass, hWndReg, hWnd, ReqID);
}

HRESULT extern WINAPI WFPExecute(HSERVICE hService, DWORD dwCommand, LPVOID lpCmdData,
	DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID) {
	LOG.info(_T("WFPExecute: hService=%d,dwCommand=%d,lpCmdData=%p,dwTimeOut=%d,hWnd=%p,ReqID=%lu"), hService, dwCommand, lpCmdData, dwTimeOut, hWnd, ReqID);
	return GetProxy().WFPExecute(hService, dwCommand, lpCmdData, dwTimeOut, hWnd, ReqID);
}

HRESULT extern WINAPI WFPGetInfo(HSERVICE hService, DWORD dwCategory,
	LPVOID lpQueryDetails, DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID) {
	LOG.info(_T("WFPGetInfo: hService=%d,dwCategory=%d,lpQueryDetails=%p,dwTimeOut=%d,hWnd=%p,ReqID=%lu"), hService, dwCategory, lpQueryDetails, dwTimeOut, hWnd, ReqID);
	return GetProxy().WFPGetInfo(hService, dwCategory, lpQueryDetails, dwTimeOut, hWnd, ReqID);
}

HRESULT extern WINAPI WFPLock(HSERVICE hService, DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID) {
	LOG.info(_T("WFPLock: hService=%d,dwTimeOut=%d,hWnd=%p,ReqID=%lu"), hService, dwTimeOut, hWnd, ReqID);
	return GetProxy().WFPLock(hService, dwTimeOut, hWnd, ReqID);
}

HRESULT extern WINAPI WFPOpen(HSERVICE hService, LPSTR lpszLogicalName, HAPP hApp, LPSTR lpszAppID, DWORD dwTraceLevel, DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID, HPROVIDER hProvider, DWORD dwSPIVersionsRequired, LPWFSVERSION lpSPIVersion, DWORD dwSrvcVersionsRequired, LPWFSVERSION lpSrvcVersion) {
	LOG.info(_T("WFPOpen: hService=%d,lpszLogicalName=%hs,hApp=%p,lpszAppID=%hs,dwTraceLevel=%d,dwTimeOut=%d,hWnd=%p,ReqID=%lu,hProvider=%p,dwSPIVersionsRequired=%x,lpSPIVersion=%p,dwSrvcVersionsRequired=%x,lpSrvcVersion=%p"), hService, lpszLogicalName, hApp, lpszAppID, dwTraceLevel, dwTimeOut, hWnd, ReqID, hProvider, dwSPIVersionsRequired, lpSPIVersion, dwSrvcVersionsRequired, lpSrvcVersion);
	return GetProxy().WFPOpen(hService, lpszLogicalName, hApp, lpszAppID, dwTraceLevel, dwTimeOut, hWnd, ReqID, hProvider, dwSPIVersionsRequired, lpSPIVersion, dwSrvcVersionsRequired, lpSrvcVersion);
}

HRESULT extern WINAPI WFPRegister(HSERVICE hService, DWORD dwEventClass, HWND hWndReg,
	HWND hWnd, REQUESTID ReqID) {
	LOG.info(_T("WFPRegister: hService=%d,dwEventClass=%d,hWndReg=%p,hWnd=%p,ReqID=%lu"), hService, dwEventClass, hWndReg, hWnd, ReqID);
	return GetProxy().WFPRegister(hService, dwEventClass, hWndReg, hWnd, ReqID);
}

HRESULT extern WINAPI WFPSetTraceLevel(HSERVICE hService, DWORD dwTraceLevel) {
	LOG.info( _T("WFPSetTraceLevel: hService=%d,dwTraceLevel=%d"), hService, dwTraceLevel);
	return GetProxy().WFPSetTraceLevel(hService, dwTraceLevel);
}

HRESULT extern WINAPI WFPUnloadService() {
	LOG.info( _T("WFPUnloadService"));
	return GetProxy().WFPUnloadService();
}

HRESULT extern WINAPI WFPUnlock(HSERVICE hService, HWND hWnd, REQUESTID ReqID) {
	LOG.info( _T("WFPUnlock: hService=%d,hWnd=%p,ReqID=%lu"), hService, hWnd, ReqID);
	return GetProxy().WFPUnlock(hService, hWnd, ReqID);
}