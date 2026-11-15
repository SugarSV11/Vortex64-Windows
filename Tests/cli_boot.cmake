# ctest script: boots vortex64-cli with a header-only ROM for a few seconds.
# Passes when the core initialises, the plugins load, the ROM loads and emulation starts, and nothing crashes.
# Usage: cmake -DCLI=<vortex64-cli.exe> -DROM=<fake.z64> [-DMODE=--interpreter|--recompiler] -P cli_boot.cmake
# isolated user data: no language/plugin/ROM settings of the user may influence the run
string(RANDOM LENGTH 8 TAG)
set(ISO "$ENV{TEMP}/vortex64-cli-boot-${TAG}")
file(MAKE_DIRECTORY "${ISO}/appdata" "${ISO}/localappdata")
set(ENV{APPDATA} "${ISO}/appdata")
set(ENV{LOCALAPPDATA} "${ISO}/localappdata")
execute_process(COMMAND "${CLI}" "${ROM}" ${MODE}
    TIMEOUT 12
    OUTPUT_VARIABLE OUT ERROR_VARIABLE ERR RESULT_VARIABLE RES)
set(ALL "${OUT}\n${ERR}")
foreach(NEEDLE "AppInit() OK" "Video plugin loaded" "ROM loaded and running" "Emulation started")
    string(FIND "${ALL}" "${NEEDLE}" POS)
    if(POS EQUAL -1)
        message(FATAL_ERROR "missing in output: ${NEEDLE}\n${ALL}")
    endif()
endforeach()
string(FIND "${ALL}" "[CRASH]" CRASH)
if(NOT CRASH EQUAL -1)
    message(FATAL_ERROR "crash detected:\n${ALL}")
endif()
file(REMOVE_RECURSE "${ISO}")
message(STATUS "cli boot OK (${MODE}); result: ${RES}")
