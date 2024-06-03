cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
	# Enable Hot Reload for MSVC compilers if supported.
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()
cmake_policy(SET CMP0060 NEW)

project(GPUInstancing LANGUAGES CXX)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/third_party.cmake")

add_subdirectory ("${CMAKE_CURRENT_SOURCE_DIR}/src/Vulkan/")
