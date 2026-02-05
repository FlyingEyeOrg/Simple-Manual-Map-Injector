// CoreCLRHostWrapper.h
#pragma once

#include "coreclrhost.h"

#include <windows.h>
#include <string>
#include <unordered_map>

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

public:
	// 默认构造函数 - 需要后续调用 Load(HMODULE)
	CoreCLRHostWrapper()
		: m_hCoreClr(nullptr), m_bInitialized(false)
		, m_coreclr_initialize(nullptr)
		, m_coreclr_set_error_writer(nullptr)
		, m_coreclr_shutdown(nullptr)
		, m_coreclr_shutdown_2(nullptr)
		, m_coreclr_create_delegate(nullptr)
		, m_coreclr_execute_assembly(nullptr)
	{
	}

	// 构造函数 - 直接传入已获取的 HMODULE
	explicit CoreCLRHostWrapper(HMODULE hCoreClr)
		: m_hCoreClr(hCoreClr), m_bInitialized(false)
		, m_coreclr_initialize(nullptr)
		, m_coreclr_set_error_writer(nullptr)
		, m_coreclr_shutdown(nullptr)
		, m_coreclr_shutdown_2(nullptr)
		, m_coreclr_create_delegate(nullptr)
		, m_coreclr_execute_assembly(nullptr)
	{
		if (m_hCoreClr)
		{
			m_bInitialized = GetExportedFunctions();
		}
	}

	// 构造函数 - 传入 coreclr.dll 路径（向后兼容）
	explicit CoreCLRHostWrapper(const char* coreclrPath)
		: m_hCoreClr(nullptr), m_bInitialized(false)
		, m_coreclr_initialize(nullptr)
		, m_coreclr_set_error_writer(nullptr)
		, m_coreclr_shutdown(nullptr)
		, m_coreclr_shutdown_2(nullptr)
		, m_coreclr_create_delegate(nullptr)
		, m_coreclr_execute_assembly(nullptr)
	{
		Load(coreclrPath);
	}

	~CoreCLRHostWrapper()
	{
		// 注意：不释放 HMODULE，因为它是外部传入的
		// 只重置函数指针
		ResetFunctionPointers();
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

	// 从路径加载 coreclr.dll（可选功能）
	bool Load(const char* coreclrPath)
	{
		if (m_bInitialized)
			return true;

		if (!coreclrPath || strlen(coreclrPath) == 0)
		{
			// 尝试获取已加载的模块
			m_hCoreClr = ::GetModuleHandleA("coreclr.dll");
			if (m_hCoreClr == nullptr)
			{
				SetLastError(ERROR_MOD_NOT_FOUND);
				return false;
			}
		}
		else
		{
			m_hCoreClr = ::LoadLibraryA(coreclrPath);
			if (m_hCoreClr == nullptr)
				return false;
		}

		m_bInitialized = GetExportedFunctions();

		if (!m_bInitialized)
		{
			::FreeLibrary(m_hCoreClr);
			m_hCoreClr = nullptr;
		}

		return m_bInitialized;
	}

	// 重置函数指针（不释放 HMODULE）
	void Reset()
	{
		ResetFunctionPointers();
		m_bInitialized = false;
	}

	// 重新绑定函数（当 HMODULE 改变时）
	bool Rebind(HMODULE hCoreClr)
	{
		Reset();
		return Load(hCoreClr);
	}

	// 检查是否已初始化
	bool IsInitialized() const { return m_bInitialized; }
	HMODULE GetModuleHandle() const { return m_hCoreClr; }

	// 获取函数地址（手动方式）
	void* GetFunctionAddress(const char* functionName)
	{
		if (!m_hCoreClr)
			return nullptr;

		return ::GetProcAddress(m_hCoreClr, functionName);
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

		// 验证所有必需的函数都已找到
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
	}
};