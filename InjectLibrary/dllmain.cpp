// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <iostream>
#include "CoreCLRHostWrapper.h"
#include <mscoree.h>



extern "C" __declspec(dllexport) void PrintHello()
{
	std::cout << "Hello world!" << std::endl;
}
class Assembly {};
typedef Assembly* (*print_func_t)(const char*);

using host_handle_t = void*;
using domain_id_t = std::uint32_t;

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	std::cout << "app is inject..." << std::endl;
	auto hHostPolicy = ::GetModuleHandleA("hostpolicy.dll");
	auto hCoreClr = ::GetModuleHandleA("coreclr.dll");

	if (!hHostPolicy || !hCoreClr) {
		std::cout << "hostpolicy.dll or coreclr.dll not found in process modules" << std::endl;
	}
	else {

		// 计算 g_context 地址
		UINT_PTR moduleBase = reinterpret_cast<UINT_PTR>(hHostPolicy);
		UINT_PTR coreclrBase = reinterpret_cast<UINT_PTR>(hCoreClr);
		// 模块基地址 + 内存区域偏移 + 内存区域内偏移
		UINT_PTR g_context_shared_address = moduleBase + 0x5E000 + 0x1B38; // shared_ptr*
		UINT_PTR g_context_address = *((UINT_PTR*)g_context_shared_address);      // g_context*
		auto coreclr_shared_address = g_context_address + 0x110; // shared_ptr*
		auto coreclr_address = *((UINT_PTR*)coreclr_shared_address); // coreclr*
		auto host_handle_address = coreclr_address + 0x58;
		auto domain_id_address = coreclr_address + 0x60;

		host_handle_t host_handle = *((host_handle_t*)host_handle_address);
		domain_id_t domain_id = *((domain_id_t*)domain_id_address);

		std::cout << "hostpolicy.dll base: 0x" << std::hex << moduleBase << std::endl;
		std::cout << "coreclr.dll base: 0x" << std::hex << coreclrBase << std::endl;

		std::cout << "g_context address: 0x" << std::hex << g_context_address << std::endl;
		std::cout << "coreclr address: 0x" << std::hex << coreclr_address << std::endl;
		std::cout << "host_handle value: 0x" << std::hex << host_handle << std::endl;
		std::cout << "domain_id address: 0x" << std::hex << domain_id << std::endl;

		CoreCLRHostWrapper wrapper(hCoreClr);
		wrapper.InitializeClrHost();

		void* func_ptr = nullptr;
		auto hr = wrapper.coreclr_create_delegate_wrapper(
			host_handle,
			domain_id,
			"WpfApp1",
			"WpfApp1.MainWindow",
			"Print",
			&func_ptr);

		auto host = wrapper.GetClrRuntimeHost();

		if (host != NULL)
		{
			std::cout << "Get CLR Runtime Host succeeded: 0x" << std::hex << host << std::endl;

			auto result = host->ExecuteInDefaultAppDomain(
				L"C:\\Users\\admin\\Desktop\\cppsamples\\Simple-Manual-Map-Injector\\ClassLibrary\\bin\\Debug\\net8.0\\ClassLibrary.dll",
				L"ClassLibrary.MainWindow",
				L"Print",
				L"123131",
				nullptr);

			if (result == S_OK)
			{
				std::cout << "ExecuteInDefaultAppDomain succeeded." << std::endl;
			}
			else
			{
				std::cout << "ExecuteInDefaultAppDomain failed. HRESULT: 0x" << std::hex << result << std::endl;
			}
		}
		else {
			std::cout << "Get CLR Runtime Host failed." << std::endl;
		}

		//auto hr = wrapper.coreclr_create_delegate_wrapper(
		//	host_handle,
		//	domain_id,
		//	"ClassLibrary",
		//	"ClassLibrary.MainWindow",
		//	"Print",
		//	&func_ptr);

		//auto hr = wrapper.coreclr_create_delegate_wrapper(
		//	host_handle,
		//	domain_id,
		//	"System.Private.CoreLib",
		//	"System.Reflection.Assembly",
		//	"LoadFile",
		//	&func_ptr);

		//if (hr != 0 || func_ptr == nullptr) {
		//	std::cout << "coreclr_create_delegate_wrapper failed: 0x" << std::hex << hr << std::endl;
		//}
		//else {
		//	std::cout << "coreclr_create_delegate_wrapper succeeded, print_func: 0x" << std::hex << func_ptr << std::endl;
		//	auto loadFile = reinterpret_cast<print_func_t>(func_ptr);
		//	//loadFile("sfsfdsf");
		//	//loadFile("C:\\Users\\admin\\Desktop\\cppsamples\\Simple-Manual-Map-Injector\\ClassLibrary\\bin\\Debug\\net8.0\\ClassLibrary.dll");
		//}
	}

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
