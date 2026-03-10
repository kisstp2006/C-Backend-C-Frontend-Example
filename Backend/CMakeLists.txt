cmake_minimum_required(VERSION 3.10)
project(Backend)

add_library(Backend SHARED Backend.cpp)
target_compile_definitions(Backend PRIVATE BACKEND_EXPORTS)
set_target_properties(Backend PROPERTIES PREFIX "" SUFFIX ".dll")
target_include_directories(Backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(EngineHost WIN32
	EngineMain.cpp
	Dx11Renderer.cpp
	HostFxrBridge.cpp)

target_include_directories(EngineHost PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(EngineHost PRIVATE cxx_std_17)
target_link_libraries(EngineHost PRIVATE Backend d3d11 dxgi)

