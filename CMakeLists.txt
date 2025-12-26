cmake_minimum_required(VERSION 3.10)

project(lab-cv)
set(LAB1_SRC_LIST src/*.cpp)

include(CTest)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23 -Wall -g")

add_executable(HaarTestExec     src/HaarTest.cpp)
add_executable(FrameSequenceTestExec     src/FrameSequenceTest.cpp)

target_include_directories(HaarTestExec			PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(FrameSequenceTestExec			PUBLIC ${PROJECT_SOURCE_DIR}/include)

add_subdirectory(PATypes)

target_link_libraries(HaarTestExec  PATypes)
target_link_libraries(FrameSequenceTestExec PATypes)

enable_testing()

add_test(success_HaarTestExec	HaarTestExec)
add_test(success_FrameSequenceTestExec	FrameSequenceTestExec)