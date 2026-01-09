cmake_minimum_required(VERSION 3.10)

project(lab-cv)
set(LABCV_SRC_LIST src/*.cpp)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2_image REQUIRED)
find_package(GLEW REQUIRED)
find_package(PkgConfig REQUIRED)
if (PkgConfig_FOUND)
	pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavdevice libavformat libavcodec libswresample libswscale libavutil)
endif()
include_directories(lab-cv ${SDL2_INCLUDE_DIRS})

include(CTest)

#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
#	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
#	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
#endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23 -Wall -g")

add_executable(HaarTestExec     			src/HaarTest.cpp)
add_executable(FrameSequenceTestExec     	src/FrameSequenceTest.cpp)
add_executable(UI							src/UI.cpp)

add_subdirectory(include/contrib/imgui)
add_subdirectory(PATypes)
add_subdirectory(include/contrib/portable-file-dialogs)

target_include_directories(HaarTestExec						PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(FrameSequenceTestExec			PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(UI								PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(UI 								PUBLIC ${IMGUI_ROOT})
target_include_directories(UI 								PUBLIC ${FFMPEG})

target_link_libraries(HaarTestExec			PATypes)
target_link_libraries(FrameSequenceTestExec PATypes)
target_link_libraries(UI					PATypes)
target_link_libraries(UI					imgui imgui_impl_sdl2 imgui_impl_opengl3 SDL2::SDL2 SDL2::SDL2main GLEW)
target_link_libraries(UI					PkgConfig::FFMPEG)
target_link_libraries(UI					portable_file_dialogs)
target_link_libraries(UI 					${SDL2_LIBRARIES})
target_link_libraries(UI 					SDL2_image)

enable_testing()

add_test(success_HaarTestExec	HaarTestExec)
add_test(success_FrameSequenceTestExec	FrameSequenceTestExec)