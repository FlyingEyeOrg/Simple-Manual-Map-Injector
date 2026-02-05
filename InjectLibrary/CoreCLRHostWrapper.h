#pragma once

#include "coreclrhost.h"

#include <windows.h>
#include <unknwn.h>      // IUnknown 接口定义
#include <objbase.h>     // COM 基础类型和函数

#include <string>
#include <vector>
#include <memory>
#include <MSCorEE.h>


// 定义 GetCLRRuntimeHost 函数指针类型
typedef HRESULT(__stdcall* GetCLRRuntimeHostPtr)(REFIID riid, IUnknown** ppUnk);

class CoreCLRHostWrapper
{
private:
	HMODULE m_hCoreClr;
	bool m_bInitialized;

	// 函数指针声明
	coreclr_initialize_ptr m_coreclr_initialize;
	coreclr_set_error_writer_ptr m_coreclr_set_error_writer;
	coreclr_shutdown_ptr m_coreclr_shutdown;
	coreclr_shutdown_2_ptr m_coreclr_shutdown_2;
	coreclr_create_delegate_ptr m_coreclr_create_delegate;
	coreclr_execute_assembly_ptr m_coreclr_execute_assembly;
	GetCLRRuntimeHostPtr m_get_clr_runtime_host;  // 新增

	// ICLRRuntimeHost 相关成员
	ICLRRuntimeHost* m_pClrRuntimeHost;
	bool m_bClrHostInitialized;

public:
	// 默认构造函数 - 需要后续调用 Load(HMODULE)
	CoreCLRHostWrapper()
		: m_hCoreClr(nullptr), m_bInitialized(false), m_bClrHostInitialized(false)
		, m_coreclr_initialize(nullptr)
		, m_coreclr_set_error_writer(nullptr)
		, m_coreclr_shutdown(nullptr)
		, m_coreclr_shutdown_2(nullptr)
		, m_coreclr_create_delegate(nullptr)
		, m_coreclr_execute_assembly(nullptr)
		, m_get_clr_runtime_host(nullptr)
		, m_pClrRuntimeHost(nullptr)
	{
	}

	// 构造函数 - 直接传入已获取的 HMODULE
	explicit CoreCLRHostWrapper(HMODULE hCoreClr)
		: m_hCoreClr(hCoreClr), m_bInitialized(false), m_bClrHostInitialized(false)
		, m_coreclr_initialize(nullptr)
		, m_coreclr_set_error_writer(nullptr)
		, m_coreclr_shutdown(nullptr)
		, m_coreclr_shutdown_2(nullptr)
		, m_coreclr_create_delegate(nullptr)
		, m_coreclr_execute_assembly(nullptr)
		, m_get_clr_runtime_host(nullptr)
		, m_pClrRuntimeHost(nullptr)
	{
		if (m_hCoreClr)
		{
			m_bInitialized = GetExportedFunctions();
		}
	}

	~CoreCLRHostWrapper()
	{
		ShutdownClrHost();
		ResetFunctionPointers();
		// 注意：不释放 HMODULE，因为它是外部传入的
	}

	// 使用已有的 HMODULE 初始化
	bool Load(HMODULE hCoreClr)
	{
		if (m_bInitialized)
			return true;

		if (!hCoreClr)
		{
			SetLastError(ERROR_INVALID_HANDLE);
			return false;
		}

		m_hCoreClr = hCoreClr;
		m_bInitialized = GetExportedFunctions();

		return m_bInitialized;
	}

	// 重置函数指针（不释放 HMODULE）
	void Reset()
	{
		ShutdownClrHost();
		ResetFunctionPointers();
		m_bInitialized = false;
		m_bClrHostInitialized = false;
	}

	// 重新绑定函数（当 HMODULE 改变时）
	bool Rebind(HMODULE hCoreClr)
	{
		Reset();
		return Load(hCoreClr);
	}

	// 检查是否已初始化
	bool IsInitialized() const { return m_bInitialized; }
	bool IsClrHostInitialized() const { return m_bClrHostInitialized; }
	HMODULE GetModuleHandle() const { return m_hCoreClr; }
	ICLRRuntimeHost* GetClrRuntimeHost() const { return m_pClrRuntimeHost; }

	// 获取函数地址（手动方式）
	void* GetFunctionAddress(const char* functionName)
	{
		if (!m_hCoreClr)
			return nullptr;

		return ::GetProcAddress(m_hCoreClr, functionName);
	}

	// 新增：获取 GetCLRRuntimeHost 函数
	GetCLRRuntimeHostPtr GetGetCLRRuntimeHost() const { return m_get_clr_runtime_host; }

	// 新增：初始化 ICLRRuntimeHost
	HRESULT InitializeClrHost()
	{
		if (m_bClrHostInitialized)
			return S_OK;

		if (!m_get_clr_runtime_host)
		{
			// 如果 GetCLRRuntimeHost 函数指针为空，尝试手动获取
			m_get_clr_runtime_host = (GetCLRRuntimeHostPtr)::GetProcAddress(m_hCoreClr, "GetCLRRuntimeHost");
			if (!m_get_clr_runtime_host)
			{
				std::cout << "GetCLRRuntimeHost function not found in coreclr.dll" << std::endl;
				return E_FAIL;
			}
			else {
				std::cout << "Successfully obtained GetCLRRuntimeHost function." << std::endl;
			}
		}

		// 使用 IID_ICLRRuntimeHost 获取 ICLRRuntimeHost 接口
		HRESULT hr = m_get_clr_runtime_host(IID_ICLRRuntimeHost, (IUnknown**)&m_pClrRuntimeHost);
		if (FAILED(hr))
		{
			std::cout << "Failed to get ICLRRuntimeHost interface. HRESULT: " << std::hex << hr << std::endl;
			m_pClrRuntimeHost = nullptr;
			return hr;
		}
		else {
			std::cout << "Successfully obtained ICLRRuntimeHost interface." << std::endl;
		}

		m_bClrHostInitialized = true;
		return S_OK;
	}

	// 新增：关闭 ICLRRuntimeHost
	HRESULT ShutdownClrHost()
	{
		if (m_pClrRuntimeHost && m_bClrHostInitialized)
		{
			m_pClrRuntimeHost->Stop();
			m_pClrRuntimeHost->Release();
			m_pClrRuntimeHost = nullptr;
			m_bClrHostInitialized = false;
		}
		return S_OK;
	}

	// 新增：执行托管代码（使用 ICLRRuntimeHost）
	HRESULT ExecuteAssemblyUsingClrHost(const wchar_t* assemblyPath, const wchar_t* typeName, const wchar_t* methodName)
	{
		if (!m_bClrHostInitialized || !m_pClrRuntimeHost)
		{
			return E_FAIL;
		}

		DWORD exitCode = 0;
		HRESULT hr = m_pClrRuntimeHost->ExecuteInDefaultAppDomain(
			assemblyPath,
			typeName,
			methodName,
			L"arg1 arg2",  // 参数
			&exitCode
		);

		return hr;
	}

	// 新增：直接调用 GetCLRRuntimeHost 的静态方法
	static HRESULT GetCLRRuntimeHostDirectly(HMODULE hCoreClr, REFIID riid, IUnknown** ppUnk)
	{
		if (!hCoreClr)
			return E_INVALIDARG;

		GetCLRRuntimeHostPtr getHost = (GetCLRRuntimeHostPtr)::GetProcAddress(hCoreClr, "GetCLRRuntimeHost");
		if (!getHost)
			return E_FAIL;

		return getHost(riid, ppUnk);
	}

	// 导出的函数调用接口
	int coreclr_initialize_wrapper(const char* exePath, const char* appDomainFriendlyName,
		int propertyCount, const char** propertyKeys, const char** propertyValues,
		void** hostHandle, unsigned int* domainId)
	{
		if (!m_coreclr_initialize)
			return -1;
		return m_coreclr_initialize(exePath, appDomainFriendlyName, propertyCount,
			propertyKeys, propertyValues, hostHandle, domainId);
	}

	int coreclr_set_error_writer_wrapper(coreclr_error_writer_callback_fn errorWriter)
	{
		if (!m_coreclr_set_error_writer)
			return -1;
		return m_coreclr_set_error_writer(errorWriter);
	}

	int coreclr_shutdown_wrapper(void* hostHandle, unsigned int domainId)
	{
		if (!m_coreclr_shutdown)
			return -1;
		return m_coreclr_shutdown(hostHandle, domainId);
	}

	int coreclr_shutdown_2_wrapper(void* hostHandle, unsigned int domainId, int* latchedExitCode)
	{
		if (!m_coreclr_shutdown_2)
			return -1;
		return m_coreclr_shutdown_2(hostHandle, domainId, latchedExitCode);
	}

	int coreclr_create_delegate_wrapper(void* hostHandle, unsigned int domainId,
		const char* entryPointAssemblyName, const char* entryPointTypeName,
		const char* entryPointMethodName, void** delegate)
	{
		if (!m_coreclr_create_delegate)
			return -1;
		return m_coreclr_create_delegate(hostHandle, domainId, entryPointAssemblyName,
			entryPointTypeName, entryPointMethodName, delegate);
	}

	int coreclr_execute_assembly_wrapper(void* hostHandle, unsigned int domainId,
		int argc, const char** argv, const char* managedAssemblyPath,
		unsigned int* exitCode)
	{
		if (!m_coreclr_execute_assembly)
			return -1;
		return m_coreclr_execute_assembly(hostHandle, domainId, argc, argv,
			managedAssemblyPath, exitCode);
	}

	// 获取原始函数指针
	coreclr_initialize_ptr GetCoreClrInitialize() const { return m_coreclr_initialize; }
	coreclr_set_error_writer_ptr GetCoreClrSetErrorWriter() const { return m_coreclr_set_error_writer; }
	coreclr_shutdown_ptr GetCoreClrShutdown() const { return m_coreclr_shutdown; }
	coreclr_shutdown_2_ptr GetCoreClrShutdown2() const { return m_coreclr_shutdown_2; }
	coreclr_create_delegate_ptr GetCoreClrCreateDelegate() const { return m_coreclr_create_delegate; }
	coreclr_execute_assembly_ptr GetCoreClrExecuteAssembly() const { return m_coreclr_execute_assembly; }

private:
	// 获取所有导出函数地址
	bool GetExportedFunctions()
	{
		// 使用 GetProcAddress 手动获取每个函数地址
		m_coreclr_initialize = (coreclr_initialize_ptr)::GetProcAddress(m_hCoreClr, "coreclr_initialize");
		m_coreclr_set_error_writer = (coreclr_set_error_writer_ptr)::GetProcAddress(m_hCoreClr, "coreclr_set_error_writer");
		m_coreclr_shutdown = (coreclr_shutdown_ptr)::GetProcAddress(m_hCoreClr, "coreclr_shutdown");
		m_coreclr_shutdown_2 = (coreclr_shutdown_2_ptr)::GetProcAddress(m_hCoreClr, "coreclr_shutdown_2");
		m_coreclr_create_delegate = (coreclr_create_delegate_ptr)::GetProcAddress(m_hCoreClr, "coreclr_create_delegate");
		m_coreclr_execute_assembly = (coreclr_execute_assembly_ptr)::GetProcAddress(m_hCoreClr, "coreclr_execute_assembly");

		// 新增：获取 GetCLRRuntimeHost 函数
		m_get_clr_runtime_host = (GetCLRRuntimeHostPtr)::GetProcAddress(m_hCoreClr, "GetCLRRuntimeHost");

		// 验证核心函数是否已找到（GetCLRRuntimeHost 是可选的，取决于使用方式）
		if (!m_coreclr_initialize || !m_coreclr_set_error_writer || !m_coreclr_shutdown ||
			!m_coreclr_shutdown_2 || !m_coreclr_create_delegate || !m_coreclr_execute_assembly)
		{
			return false;
		}

		return true;
	}

	// 重置函数指针（不释放 HMODULE）
	void ResetFunctionPointers()
	{
		m_coreclr_initialize = nullptr;
		m_coreclr_set_error_writer = nullptr;
		m_coreclr_shutdown = nullptr;
		m_coreclr_shutdown_2 = nullptr;
		m_coreclr_create_delegate = nullptr;
		m_coreclr_execute_assembly = nullptr;
		m_get_clr_runtime_host = nullptr;  // 新增
	}
};