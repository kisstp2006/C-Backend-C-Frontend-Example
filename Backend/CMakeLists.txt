cmake_minimum_required(VERSION 3.10)
project(Backend)

set(IMGUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui")

add_library(Backend SHARED Backend.cpp)
target_compile_definitions(Backend PRIVATE BACKEND_EXPORTS)
set_target_properties(Backend PROPERTIES PREFIX "" SUFFIX ".dll")
target_include_directories(Backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(EngineHost WIN32
	EngineMain.cpp
	Dx11Renderer.cpp
	HostFxrBridge.cpp
	ImGuiLayer.cpp)

target_include_directories(EngineHost PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(EngineHost PRIVATE cxx_std_17)
target_link_libraries(EngineHost PRIVATE Backend d3d11 dxgi)

if (EXISTS "${IMGUI_ROOT}/imgui.h")
	target_compile_definitions(EngineHost PRIVATE ENGINEHOST_WITH_IMGUI=1)
	target_include_directories(EngineHost PRIVATE ${IMGUI_ROOT} ${IMGUI_ROOT}/backends)
	target_sources(EngineHost PRIVATE
		${IMGUI_ROOT}/imgui.cpp
		${IMGUI_ROOT}/imgui_draw.cpp
		${IMGUI_ROOT}/imgui_tables.cpp
		${IMGUI_ROOT}/imgui_widgets.cpp
		${IMGUI_ROOT}/backends/imgui_impl_win32.cpp
		${IMGUI_ROOT}/backends/imgui_impl_dx11.cpp)
	message(STATUS "EngineHost: Dear ImGui support enabled")
else()
	target_compile_definitions(EngineHost PRIVATE ENGINEHOST_WITH_IMGUI=0)
	message(STATUS "EngineHost: Dear ImGui support disabled (missing ${IMGUI_ROOT})")
endif()

