#include "o2xfs-proxy.h"
#include "o2xfs-common.h"
#include "o2xfs-logger.h"

JavaVM *jvm = 0;

jobject serviceProviderObject = NULL;
jmethodID cancelAsyncRequestMethod = NULL;
jmethodID closeMethod = NULL;
jmethodID deregisterMethod = NULL;
jmethodID executeMethod = NULL;
jmethodID getInfoMethod = NULL;
jmethodID lockMethod = NULL;
jmethodID openMethod = NULL;
jmethodID registerMethod = NULL;
jmethodID setTraceLevelMethod = NULL;
jmethodID unloadServiceMethod = NULL;
jmethodID unlockMethod = NULL;

TCHAR* LocateJRE(TCHAR* jreKeyName);
TCHAR* QueryRuntimeLib(HKEY jreKey, TCHAR* keyName);
BOOL LoadJavaVM(TCHAR* jvmpath, InvocationFunctions*);
jboolean GetCreatedJVM(InvocationFunctions *ifn);
TCHAR* GetIniFile(TCHAR* program);
void ReadJavaVMInitArgs(TCHAR* Filename, JavaVMInitArgs *args);
jboolean InitializeJVM(TCHAR* program, InvocationFunctions *ifn);
jbyteArray NewBuffer(JNIEnv* env, void* address, jlong capacity);
jboolean GetEnv(void **env);

extern o2xfs::Logger LOG;

TCHAR* LocateJRE(TCHAR* jreKeyName) {
	TCHAR* result = NULL;
	HKEY jreKey = NULL;
	DWORD length = MAX_PATH;
	TCHAR keyName[MAX_PATH];
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, jreKeyName, 0, KEY_READ, &jreKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(jreKey, _T("CurrentVersion"), NULL, NULL, (LPBYTE) &keyName, &length) == ERROR_SUCCESS) {
			result = QueryRuntimeLib(jreKey, keyName);
		}
		RegCloseKey(jreKey);
	}
	return result;
}

TCHAR* QueryRuntimeLib(HKEY jreKey, TCHAR* keyName) {
	TCHAR* result = NULL;
	HKEY subKey = NULL;
	TCHAR value[MAX_PATH];
	DWORD length = MAX_PATH;
	if (RegOpenKeyEx(jreKey, keyName, 0, KEY_READ, &subKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(subKey, _T("RuntimeLib"), NULL, NULL, (LPBYTE) &value, &length) == ERROR_SUCCESS) {
			result = _tcsdup(value);
		}
		RegCloseKey(subKey);
	}
	return result;
}

BOOL LoadJavaVM(TCHAR* jvmpath, InvocationFunctions *ifn) {
	HINSTANCE hInstance = LoadLibrary(jvmpath);
	if (hInstance == NULL) {
		LOG.error(_T("Error loading: %s (%d)"), jvmpath, GetLastError());
		return FALSE;
	}

	ifn->CreateJavaVM = (CreateJavaVM_t) GetProcAddress(hInstance, "JNI_CreateJavaVM");
	ifn->GetCreatedJavaVMs = (GetCreatedJavaVMs_t) GetProcAddress(hInstance, "JNI_GetCreatedJavaVMs");
	if (ifn->CreateJavaVM == NULL || ifn->GetCreatedJavaVMs == 0) {
		LOG.error(_T("can't find JNI interfaces in: %s (%d)"), jvmpath, GetLastError());
		return FALSE;
	}
	return TRUE;
}

TCHAR* GetIniFile(TCHAR* program) {
	TCHAR* result = (TCHAR*) malloc((_tcslen(program) + 5) * sizeof(TCHAR));
	_tcscpy(result, program);
	TCHAR* extension = _tcsrchr(result, _T('.'));
	_tcscpy(extension, _T(".ini"));
	return result;
}

void ReadJavaVMInitArgs(TCHAR* Filename, JavaVMInitArgs *args) {
	FILE *file = NULL;
	TCHAR *buffer;
	int maxArgs = 128;
	size_t bufferSize = 1024;
	size_t length;

	file = _tfopen(Filename, _T("rt"));
	if (file == NULL) {
		LOG.error(_T("Error opening file: %s"), Filename);
		return;
	}

	LOG.info(_T("Loading: %s"), Filename);

	buffer = (TCHAR*) malloc(bufferSize * sizeof(TCHAR));
	args->options = (JavaVMOption*) malloc(maxArgs * sizeof(JavaVMOption));

	args->nOptions = 0;

	while (_fgetts(buffer, bufferSize, file) != NULL) {
		while (buffer[bufferSize - 2] != _T('\n') && _tcslen(buffer) == (bufferSize - 1)) {
			bufferSize += 1024;
			buffer = (TCHAR*) realloc(buffer, bufferSize * sizeof(TCHAR));
			buffer[bufferSize - 2] = 0;
			if (_fgetts(buffer + bufferSize - 1025, 1025, file) == NULL) {
				break;
			}
		}
		length = _tcslen(buffer);
		while (length > 0
				&& (buffer[length - 1] == _T('\n')
						|| buffer[length - 1] == _T(' ')
						|| buffer[length - 1] == _T('\t'))) {
			buffer[--length] = 0;
		}
		if (length == 0) {
			continue;
		}

#ifdef UNICODE
		int byteCount = WideCharToMultiByte(CP_ACP, 0, (wchar_t *) buffer, -1, NULL, 0, NULL, NULL);
		args->options[args->nOptions].optionString = (char*) malloc(byteCount + 1);
		args->options[args->nOptions].optionString[byteCount] = 0;
		WideCharToMultiByte (CP_ACP, 0, (wchar_t *) buffer, -1, args->options[args->nOptions].optionString, byteCount, NULL, NULL);
#else
		args->options[args->nOptions].optionString = (char*) _tcsdup(buffer);
#endif
		LOG.debug(_T("JavaVMOption[%d]=%hs"), args->nOptions, args->options[args->nOptions].optionString);
		args->options[args->nOptions].extraInfo = 0;
		args->nOptions++;
	}
	fclose(file);
	free(buffer);
}

jboolean GetCreatedJVM(InvocationFunctions *ifn) {
	jint r;
	jsize nVMs = 0;
	r = ifn->GetCreatedJavaVMs(&jvm, 1, &nVMs);
	return r == JNI_OK && nVMs > 0;
}

jboolean InitializeJVM(TCHAR* program, InvocationFunctions *ifn) {
	jint r;
	JNIEnv *env = 0;
	JavaVMInitArgs args;
	TCHAR* config_file;

	args.version = JNI_VERSION_1_2;
	args.nOptions = 0;
	args.ignoreUnrecognized = JNI_TRUE;

	config_file = GetIniFile(program);
	ReadJavaVMInitArgs(config_file, &args);
	free(config_file);

	r = ifn->CreateJavaVM(&jvm, (void**) &env, &args);
	if (r != JNI_OK) {
		LOG.error(_T("Could not create the Java Virtual Machine."));
	}

	for (int i = 0; i < args.nOptions; i++) {
		free(args.options[i].optionString);
	}
	free(args.options);
	return r == JNI_OK;
}

jbyteArray NewBuffer(JNIEnv *env, void *address, jlong capacity) {
	jbyteArray result = NULL;
	if (address != NULL) {
		result = env->NewByteArray(capacity);
		env->SetByteArrayRegion(result, 0, capacity, (jbyte*) &address);
	}
	return result;
}

jboolean GetEnv(void **env) {
	int r = jvm->GetEnv(env, JNI_VERSION_1_2);
	LOG.info(_T("jvm->GetEnv: r=%d"), r);
	LOG.info(_T("jvm->GetEnv: JNI_EDETACHED=%d"), JNI_EDETACHED);
	if (r == JNI_EDETACHED) {
		r = jvm->AttachCurrentThread(env, NULL);
		LOG.info(_T("jvm->AttachCurrentThread: r=%d"), r);
	}
	LOG.info(_T("jvm->AttachCurrentThread: r=%d"), r);
	return r == JNI_OK;
}

BOOL Proxy::isInitialized() {
	return serviceProviderObject != NULL;
}

void Proxy::init(TCHAR* program) {
	TCHAR* jvmpath = NULL;
	DWORD nSize = GetEnvironmentVariable(_T("JAVA_HOME"), NULL, 0);
	if (nSize > 0) {
		TCHAR* clientLib = _T("\\bin\\client\\jvm.dll");
		jvmpath = new TCHAR[MAX_PATH + _tcslen(clientLib)];
		nSize = GetEnvironmentVariable(_T("JAVA_HOME"), jvmpath, nSize);
		if (nSize > 0) {
			if (_tcscmp(&jvmpath[nSize - 1], _T("\\")) == 0) {
				nSize--;
			}
			_tcscpy(&jvmpath[nSize], clientLib);
		}
	}

	if (jvmpath == NULL) {
		jvmpath = LocateJRE(_T("Software\\JavaSoft\\JRE"));
		if (jvmpath == NULL) {
			jvmpath = LocateJRE(_T("Software\\JavaSoft\\Java Runtime Environment"));
		}
	}
	if (jvmpath == NULL) {
		LOG.error(_T("Could not find Java SE Runtime Environment."));
		return;
	}

	if (jvm != NULL) {
		return;
	}

	LOG.info(_T("JVM path is %s"), jvmpath);
	JNIEnv *env = 0;
	InvocationFunctions ifn;
	if (LoadJavaVM(jvmpath, &ifn)) {		
		if (GetCreatedJVM(&ifn) || InitializeJVM(program, &ifn)) {
			jvm->AttachCurrentThread((void**) &env, NULL);
			jclass factoryClass = env->FindClass("at/o2xfs/xfs/spi/api/ServiceProviderFactory");
			jclass serviceProviderClass = env->FindClass("at/o2xfs/xfs/spi/api/ServiceProvider");
			if (factoryClass != NULL) {
				jmethodID factoryMethod = env->GetStaticMethodID(factoryClass, "getFactory", "()Lat/o2xfs/xfs/spi/api/ServiceProviderFactory;");
				if (factoryMethod != NULL) {
					jobject factoryObject = env->CallStaticObjectMethod(factoryClass, factoryMethod);
					if (factoryObject != NULL) {
						jmethodID newServiceProviderMethod = env->GetMethodID(factoryClass, "newServiceProvider", "()Lat/o2xfs/xfs/spi/api/ServiceProvider;");
						if (newServiceProviderMethod != NULL) {
							serviceProviderObject = env->NewGlobalRef(env->CallObjectMethod(factoryObject, newServiceProviderMethod));
							if (serviceProviderObject == NULL) {
								LOG.error(_T("serviceProviderObject is NULL"));
							}
						}
					}
				}
			} else {
				LOG.error(_T("ServiceProviderFactory not found"));
			}
			if (serviceProviderClass != NULL) {
				cancelAsyncRequestMethod = env->GetMethodID(serviceProviderClass, "cancelAsyncRequest", "(IJ)I");
				closeMethod = env->GetMethodID(serviceProviderClass, "close", "(I[BJ)I");
				deregisterMethod = env->GetMethodID(serviceProviderClass, "deregister", "(IJ[B[BJ)I");
				executeMethod = env->GetMethodID(serviceProviderClass, "execute", "(IJ[BJ[BJ)I");
				getInfoMethod = env->GetMethodID(serviceProviderClass, "getInfo", "(IJ[BJ[BJ)I");
				lockMethod = env->GetMethodID(serviceProviderClass, "lock", "(IJ[BJ)I");
				openMethod = env->GetMethodID(serviceProviderClass, "open", "(ILjava/lang/String;[BLjava/lang/String;JJ[BJ[BJ[BJ[B)I");
				registerMethod = env->GetMethodID(serviceProviderClass, "register", "(IJ[B[BJ)I");
				setTraceLevelMethod = env->GetMethodID(serviceProviderClass, "setTraceLevel", "(IJ)I");
				unloadServiceMethod = env->GetMethodID(serviceProviderClass, "unloadService", "()I");
				unlockMethod = env->GetMethodID(serviceProviderClass, "unlock", "(I[BJ)I");
			}
			if (env->ExceptionOccurred()) {
				env->ExceptionDescribe();
				env->ExceptionClear();
			}
			jvm->DetachCurrentThread();
		} else {
			LOG.error(_T("No JVM"));
		}
	}
}

HRESULT Proxy::WFPCancelAsyncRequest(HSERVICE hService, REQUESTID RequestID) {
	HRESULT result = WFS_ERR_INTERNAL_ERROR;
	JNIEnv *env = 0;
	if (isInitialized() && GetEnv((void**)&env) && cancelAsyncRequestMethod != NULL) {
		result = (HRESULT) env->CallIntMethod(serviceProviderObject, cancelAsyncRequestMethod, hService, RequestID);
		jvm->DetachCurrentThread();
	}
	return result;
}

HRESULT Proxy::WFPClose(HSERVICE hService, HWND hWnd, REQUESTID ReqID) {
	HRESULT result = WFS_ERR_INTERNAL_ERROR;
	JNIEnv *env = 0;
	if (isInitialized() && GetEnv((void**) &env) && closeMethod != NULL) {
		jbyteArray hWndObject = NULL;
		if (hWnd != NULL) {
			hWndObject = NewBuffer(env, hWnd, sizeof(HWND));
		}
		result = (HRESULT) env->CallIntMethod(serviceProviderObject, closeMethod, hService, hWndObject, (jlong) ReqID);
		jvm->DetachCurrentThread();
	}
	return result;
}

HRESULT Proxy::WFPDeregister(HSERVICE hService, DWORD dwEventClass, HWND hWndReg, HWND hWnd, REQUESTID ReqID) {
	HRESULT result = WFS_ERR_INTERNAL_ERROR;
	JNIEnv *env = 0;
	if (isInitialized() && GetEnv((void**) &env) && deregisterMethod != NULL) {
		jbyteArray hWndRegObject = NULL;
		jbyteArray hWndObject = NULL;
		if (hWndReg != NULL) {
			hWndRegObject = NewBuffer(env, hWndReg, sizeof(HWND));
		}
		if (hWnd != NULL) {
			hWndObject = NewBuffer(env, hWnd, sizeof(HWND));
		}
		result = (HRESULT) env->CallIntMethod(serviceProviderObject, deregisterMethod, hService, (jlong) dwEventClass, hWndRegObject, hWndObject, (jlong) ReqID);
		jvm->DetachCurrentThread();
	}
	return result;
}

HRESULT Proxy::WFPExecute(HSERVICE hService, DWORD dwCommand, LPVOID lpCmdData, DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID) {
	HRESULT result = WFS_ERR_INTERNAL_ERROR;
	JNIEnv *env = 0;
	if (isInitialized() && GetEnv((void**) &env) && executeMethod != NULL) {
		jbyteArray lpCmdDataObject = NULL;
		jbyteArray hWndObject = NULL;
		if (lpCmdData != NULL) {
			lpCmdDataObject = NewBuffer(env, lpCmdData, sizeof(LPVOID));
		}
		if (hWnd != NULL) {
			hWndObject = NewBuffer(env, hWnd, sizeof(HWND));
		}
		result = (HRESULT) env->CallIntMethod(serviceProviderObject, executeMethod, hService, (jlong) dwCommand, lpCmdDataObject, (jlong) dwTimeOut, hWndObject, (jlong) ReqID);
		jvm->DetachCurrentThread();
	}
	return result;
}

HRESULT Proxy::WFPGetInfo(HSERVICE hService, DWORD dwCategory, LPVOID lpQueryDetails, DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID) {
	HRESULT result = WFS_ERR_INTERNAL_ERROR;
	JNIEnv *env = 0;
	if (isInitialized() && GetEnv((void**) &env) && getInfoMethod != NULL) {
		jbyteArray lpQueryDetailsObject = NULL;
		jbyteArray hWndObject = NULL;
		if (lpQueryDetails != NULL) {
			lpQueryDetailsObject = NewBuffer(env, &lpQueryDetails, sizeof(LPVOID));
		}
		if (hWnd != NULL) {
			hWndObject = o2xfs::ToArray(env, hWnd);
		}
		result = (HRESULT) env->CallIntMethod(serviceProviderObject, getInfoMethod, hService, (jlong) dwCategory, lpQueryDetailsObject, (jlong) dwTimeOut, hWndObject, (jlong) ReqID);
		jvm->DetachCurrentThread();
	}
	return result;
}

HRESULT Proxy::WFPLock(HSERVICE hService, DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID) {
	HRESULT result = WFS_ERR_INTERNAL_ERROR;
	JNIEnv *env = 0;
	if (isInitialized() && GetEnv((void**) &env) && lockMethod != NULL) {
		jbyteArray hWndObject = NULL;
		if (hWnd != NULL) {
			hWndObject = NewBuffer(env, hWnd, sizeof(HWND));
		}
		result = (HRESULT) env->CallIntMethod(serviceProviderObject, lockMethod, hService, (jlong) dwTimeOut, hWndObject, (jlong) ReqID);
		jvm->DetachCurrentThread();
	}
	return result;
}

HRESULT Proxy::WFPOpen(HSERVICE hService, LPSTR lpszLogicalName, HAPP hApp, LPSTR lpszAppID, DWORD dwTraceLevel, DWORD dwTimeOut, HWND hWnd, REQUESTID ReqID, HPROVIDER hProvider, DWORD dwSPIVersionsRequired, LPWFSVERSION lpSPIVersion, DWORD dwSrvcVersionsRequired, LPWFSVERSION lpSrvcVersion) {
	HRESULT result = WFS_ERR_INTERNAL_ERROR;
	JNIEnv *env = NULL;
	if (isInitialized() && GetEnv((void**) &env) && openMethod != NULL) {
		jstring logicalNameString = NULL;
		jbyteArray hAppObject = NULL;
		jstring appIdString = NULL;
		jbyteArray hWndObject = NULL;
		jbyteArray hProviderObject = NULL;
		jbyteArray lpSPIVersionObject = NULL;
		jbyteArray lpSrvcVersionObject = NULL;

		if (lpszLogicalName != NULL) {
			logicalNameString = env->NewStringUTF(lpszLogicalName);
		}
		if (hApp != NULL) {
			hAppObject = NewBuffer(env, &hApp, sizeof(HAPP));
		}
		if (lpszAppID != NULL) {
			appIdString = env->NewStringUTF(lpszAppID);
		}
		if (hWnd != NULL) {
			hWndObject = NewBuffer(env, hWnd, sizeof(HWND));
		}
		if (hProvider != NULL) {
			hProviderObject = NewBuffer(env, &hProvider, sizeof(HPROVIDER));
		}
		if (lpSPIVersion != NULL) {
			lpSPIVersionObject = o2xfs::ToArray(env, lpSPIVersion);
		}
		if (lpSrvcVersion != NULL) {
			lpSrvcVersionObject = o2xfs::ToArray(env, lpSrvcVersion);
		}
		result = (HRESULT) env->CallIntMethod(serviceProviderObject, openMethod, hService, logicalNameString, hAppObject, appIdString, (jlong) dwTraceLevel, (jlong) dwTimeOut, hWndObject, (jlong) ReqID, hProviderObject, (jlong) dwSPIVersionsRequired, lpSPIVersionObject, (jlong) dwSrvcVersionsRequired, lpSrvcVersionObject);
		jvm->DetachCurrentThread();
	}
	return result;
}

HRESULT Proxy::WFPRegister(HSERVICE hService, DWORD dwEventClass, HWND hWndReg, HWND hWnd, REQUESTID ReqID) {
	HRESULT result = WFS_ERR_INTERNAL_ERROR;
	JNIEnv *env = 0;
	if (isInitialized() && GetEnv((void**) &env) && registerMethod != NULL) {
		jbyteArray hWndRegObject = NULL;
		jbyteArray hWndObject = NULL;
		if (hWndReg != NULL) {
			hWndRegObject = o2xfs::ToArray(env, hWndReg);
		}
		if (hWnd != NULL) {
			hWndObject = o2xfs::ToArray(env, hWndReg);
		}
		result = (HRESULT) env->CallIntMethod(serviceProviderObject, registerMethod, hService, (jlong) dwEventClass, hWndRegObject, hWndObject, (jlong) ReqID);
		jvm->DetachCurrentThread();
	}
	return result;
}

HRESULT Proxy::WFPSetTraceLevel(HSERVICE hService, DWORD dwTraceLevel) {
	HRESULT result = WFS_ERR_INTERNAL_ERROR;
	JNIEnv *env = 0;
	if (isInitialized() && GetEnv((void**) &env) && setTraceLevelMethod != NULL) {
		result = (HRESULT) env->CallIntMethod(serviceProviderObject, setTraceLevelMethod, hService, (jlong) dwTraceLevel);
		jvm->DetachCurrentThread();
	}
	return result;
}

HRESULT Proxy::WFPUnloadService() {
	HRESULT result = WFS_SUCCESS;
	JNIEnv *env = 0;
	if (isInitialized() && GetEnv((void**) &env) && unloadServiceMethod != NULL) {
		result = (HRESULT) env->CallIntMethod(serviceProviderObject, unloadServiceMethod);
		env->DeleteGlobalRef(serviceProviderObject);
		serviceProviderObject = NULL;
		jvm->DetachCurrentThread();
	}
	return result;
}

HRESULT Proxy::WFPUnlock(HSERVICE hService, HWND hWnd, REQUESTID ReqID) {
	HRESULT result = WFS_ERR_INTERNAL_ERROR;
	JNIEnv *env = 0;
	if (isInitialized() && GetEnv((void**) &env) && unlockMethod != NULL) {
		jbyteArray hWndObject = NULL;
		if (hWnd != NULL) {
			hWndObject = o2xfs::ToArray(env, hWnd);
		}
		result = (HRESULT) env->CallIntMethod(serviceProviderObject, unlockMethod, hService, hWndObject, (jlong) ReqID);
		jvm->DetachCurrentThread();
	}
	return result;
}
