#pragma once

#include <string>

class HostFxrBridge {
public:
    HostFxrBridge();
    ~HostFxrBridge();

    bool Initialize(const std::wstring& runtimeConfigPath,
                    const std::wstring& assemblyPath,
                    const std::wstring& typeName,
                    const std::wstring& startMethodName,
                    const std::wstring& updateMethodName);

    bool InvokeStart();
    bool InvokeUpdate(float deltaTimeSeconds);
    void Shutdown();

private:
    void* hostFxrModule_;
    void* context_;

    int(__stdcall* startDelegate_)(void* args, int size);
    int(__stdcall* updateDelegate_)(void* args, int size);

    int (*closeFn_)(void* host_context_handle);
};
