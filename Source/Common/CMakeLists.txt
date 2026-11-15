cmake_minimum_required(VERSION 3.16)
project(Common CXX)

set(COMMON_PORTABLE_SRC
    DateTime.cpp
    File.cpp
    HighResTimeStamp.cpp
    IniFile.cpp
    Log.cpp
    md5.cpp
    MemTest.cpp
    path.cpp
    Platform.cpp
    Random.cpp
    StdString.cpp
    Trace.cpp
    Util.cpp
)
if(WIN32)
    # Vortex64-Windows: native Win32 implementations (Source/Common/Windows)
    set(COMMON_PLATFORM_SRC
        Windows/CriticalSection_win.cpp
        Windows/DynamicLibrary_win.cpp
        Windows/MemoryManagement_win.cpp
        Windows/SyncEvent_win.cpp
        Windows/Thread_win.cpp)
else()
    set(COMMON_PLATFORM_SRC CriticalSection.cpp DynamicLibrary.cpp MemoryManagement.cpp SyncEvent.cpp Thread.cpp)
endif()
add_library(Common STATIC ${COMMON_PORTABLE_SRC} ${COMMON_PLATFORM_SRC})

target_include_directories(Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
set_target_properties(Common PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
if(WIN32)
    target_include_directories(Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Windows/compat)
    target_compile_definitions(Common PUBLIC USE_WINDOWS_API NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(Common PUBLIC shlwapi shell32 ole32)
else()
    target_link_libraries(Common PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()

# Linux threading layer regression test (SyncEvent, CThread::Terminate)
add_executable(common-test EXCLUDE_FROM_ALL tests/common_test.cpp)
set_target_properties(common-test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(common-test PRIVATE Common)

# The move from the folders of Project64 to those of Vortex64 (a copy that keeps the old ones)
add_executable(migration-test EXCLUDE_FROM_ALL tests/migration_test.cpp)
set_target_properties(migration-test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(migration-test PRIVATE Common)
