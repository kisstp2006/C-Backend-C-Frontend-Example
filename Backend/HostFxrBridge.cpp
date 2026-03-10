#include "HostFxrBridge.h"

#include <filesystem>
#include <windows.h>

#if defined(_WIN32)
#define CORECLR_DELEGATE_CALLTYPE __stdcall
#endif

using char_t = wchar_t;
using hostfxr_handle = void*;
static const char_t* const UNMANAGEDCALLERSONLY_METHOD = reinterpret_cast<const char_t*>(static_cast<intptr_t>(-1));

struct hostfxr_initialize_parameters {
    size_t size;
    const char_t* host_path;
    const char_t* dotnet_root;
};

enum hostfxr_delegate_type {
    hdt_com_activation = 0,
    hdt_load_in_memory_assembly = 1,
    hdt_winrt_activation = 2,
    hdt_com_register = 3,
    hdt_com_unregister = 4,
    hdt_load_assembly_and_get_function_pointer = 5
};

using hostfxr_initialize_for_runtime_config_fn = int(*)(
    const char_t* runtime_config_path,
    const hostfxr_initialize_parameters* parameters,
    hostfxr_handle* host_context_handle);

using hostfxr_get_runtime_delegate_fn = int(*)(
    hostfxr_handle host_context_handle,
    hostfxr_delegate_type type,
    void** delegate);

using hostfxr_close_fn = int(*)(hostfxr_handle host_context_handle);

using load_assembly_and_get_function_pointer_fn = int(*)(
    const char_t* assembly_path,
    const char_t* type_name,
    const char_t* method_name,
    const char_t* delegate_type_name,
    void* reserved,
    void** delegate);

using component_entry_point_fn = int(CORECLR_DELEGATE_CALLTYPE*)(void* args, int32_t size);

namespace {

std::wstring GetDotnetRoot() {
    wchar_t* dotnetRoot = nullptr;
    size_t len = 0;
    _wdupenv_s(&dotnetRoot, &len, L"DOTNET_ROOT");

    if (dotnetRoot != nullptr && len > 0) {
        std::wstring root(dotnetRoot);
        free(dotnetRoot);
        return root;
    }

    if (dotnetRoot != nullptr) {
        free(dotnetRoot);
    }

    return L"C:\\Program Files\\dotnet";
}

std::wstring FindLatestHostFxrPath(const std::wstring& dotnetRoot) {
    namespace fs = std::filesystem;

    fs::path fxrDir = fs::path(dotnetRoot) / L"host" / L"fxr";
    if (!fs::exists(fxrDir) || !fs::is_directory(fxrDir)) {
        return L"";
    }

    fs::path latestVersionPath;
    for (const auto& entry : fs::directory_iterator(fxrDir)) {
        if (!entry.is_directory()) {
            continue;
        }

        if (latestVersionPath.empty() || entry.path().filename().wstring() > latestVersionPath.filename().wstring()) {
            latestVersionPath = entry.path();
        }
    }

    if (latestVersionPath.empty()) {
        return L"";
    }

    fs::path hostFxrDll = latestVersionPath / L"hostfxr.dll";
    if (!fs::exists(hostFxrDll)) {
        return L"";
    }

    return hostFxrDll.wstring();
}

}  // namespace

HostFxrBridge::HostFxrBridge()
    : hostFxrModule_(nullptr),
      context_(nullptr),
      startDelegate_(nullptr),
      updateDelegate_(nullptr),
      closeFn_(nullptr) {
}

HostFxrBridge::~HostFxrBridge() {
    Shutdown();
}

bool HostFxrBridge::Initialize(const std::wstring& runtimeConfigPath,
                               const std::wstring& assemblyPath,
                               const std::wstring& typeName,
                               const std::wstring& startMethodName,
                               const std::wstring& updateMethodName) {
    Shutdown();

    const std::wstring dotnetRoot = GetDotnetRoot();
    const std::wstring hostFxrPath = FindLatestHostFxrPath(dotnetRoot);

    if (hostFxrPath.empty()) {
        return false;
    }

    HMODULE hostFxrModule = ::LoadLibraryW(hostFxrPath.c_str());
    if (hostFxrModule == nullptr) {
        return false;
    }

    hostFxrModule_ = hostFxrModule;

    auto init_fptr = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
        ::GetProcAddress(hostFxrModule, "hostfxr_initialize_for_runtime_config"));
    auto get_delegate_fptr = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
        ::GetProcAddress(hostFxrModule, "hostfxr_get_runtime_delegate"));
    auto close_fptr = reinterpret_cast<hostfxr_close_fn>(::GetProcAddress(hostFxrModule, "hostfxr_close"));
    closeFn_ = close_fptr;

    if (init_fptr == nullptr || get_delegate_fptr == nullptr || close_fptr == nullptr) {
        Shutdown();
        return false;
    }

    hostfxr_handle context = nullptr;
    hostfxr_initialize_parameters params = {};
    params.size = sizeof(hostfxr_initialize_parameters);
    params.host_path = nullptr;
    params.dotnet_root = dotnetRoot.c_str();

    int rc = init_fptr(runtimeConfigPath.c_str(), &params, &context);
    if (rc != 0 || context == nullptr) {
        Shutdown();
        return false;
    }

    context_ = context;

    load_assembly_and_get_function_pointer_fn loadAssemblyAndGetFunctionPointer = nullptr;
    rc = get_delegate_fptr(
        context,
        hdt_load_assembly_and_get_function_pointer,
        reinterpret_cast<void**>(&loadAssemblyAndGetFunctionPointer));

    if (rc != 0 || loadAssemblyAndGetFunctionPointer == nullptr) {
        Shutdown();
        return false;
    }

    component_entry_point_fn managedStart = nullptr;
    rc = loadAssemblyAndGetFunctionPointer(
        assemblyPath.c_str(),
        typeName.c_str(),
        startMethodName.c_str(),
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        reinterpret_cast<void**>(&managedStart));

    if (rc != 0 || managedStart == nullptr) {
        Shutdown();
        return false;
    }

    component_entry_point_fn managedUpdate = nullptr;
    rc = loadAssemblyAndGetFunctionPointer(
        assemblyPath.c_str(),
        typeName.c_str(),
        updateMethodName.c_str(),
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        reinterpret_cast<void**>(&managedUpdate));

    if (rc != 0 || managedUpdate == nullptr) {
        Shutdown();
        return false;
    }

    startDelegate_ = reinterpret_cast<int(__stdcall*)(void*, int)>(managedStart);
    updateDelegate_ = reinterpret_cast<int(__stdcall*)(void*, int)>(managedUpdate);

    return true;
}

bool HostFxrBridge::InvokeStart() {
    if (startDelegate_ == nullptr) {
        return false;
    }

    return startDelegate_(nullptr, 0) == 0;
}

bool HostFxrBridge::InvokeUpdate(float deltaTimeSeconds) {
    if (updateDelegate_ == nullptr) {
        return false;
    }

    return updateDelegate_(&deltaTimeSeconds, sizeof(float)) == 0;
}

void HostFxrBridge::Shutdown() {
    startDelegate_ = nullptr;
    updateDelegate_ = nullptr;

    if (closeFn_ != nullptr && context_ != nullptr) {
        closeFn_(context_);
    }

    context_ = nullptr;
    closeFn_ = nullptr;

    if (hostFxrModule_ != nullptr) {
        ::FreeLibrary(reinterpret_cast<HMODULE>(hostFxrModule_));
    }

    hostFxrModule_ = nullptr;
}
