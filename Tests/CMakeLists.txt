# Vortex64-Windows automated tests (see Docs/development/TESTING.md)
if(NOT WIN32)
    return()
endif()

add_executable(core-test core_test.cpp ${CMAKE_SOURCE_DIR}/Source/Vortex64-cli/LinuxNotification.cpp)
target_include_directories(core-test PRIVATE ${CMAKE_SOURCE_DIR}/Source)
set_target_properties(core-test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(core-test PRIVATE Common Vortex64-core)
# the plugins the application loads at start-up
add_dependencies(core-test Vortex64-video Vortex64-rsp Vortex64-audio Vortex64-input)

add_executable(rsp-test rsp_test.cpp)
target_include_directories(rsp-test PRIVATE ${CMAKE_SOURCE_DIR}/Source)
set_target_properties(rsp-test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(rsp-test PRIVATE Common)
add_dependencies(rsp-test Vortex64-rsp)

add_executable(instance-test instance_test.cpp)
target_include_directories(instance-test PRIVATE ${CMAKE_SOURCE_DIR}/Source)
set_target_properties(instance-test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(instance-test PRIVATE Common)

add_executable(make-fake-rom make_fake_rom.cpp)
set_target_properties(make-fake-rom PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(FAKE_ROM ${CMAKE_BINARY_DIR}/fake-test-rom.z64)
add_custom_command(OUTPUT ${FAKE_ROM} COMMAND make-fake-rom ${FAKE_ROM} DEPENDS make-fake-rom)
add_custom_target(fake-test-rom DEPENDS ${FAKE_ROM})

add_test(NAME common COMMAND common-test)
add_test(NAME core COMMAND core-test)
add_test(NAME rsp COMMAND rsp-test)
add_test(NAME instance COMMAND instance-test)
add_test(NAME cli-boot-interpreter COMMAND ${CMAKE_COMMAND} -DCLI=$<TARGET_FILE:vortex64-cli> -DROM=${FAKE_ROM} -DMODE=--interpreter -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_boot.cmake)
add_test(NAME cli-boot-recompiler COMMAND ${CMAKE_COMMAND} -DCLI=$<TARGET_FILE:vortex64-cli> -DROM=${FAKE_ROM} -DMODE=--recompiler -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_boot.cmake)
if(TARGET jit-fuzz-test)
    # differential test of the x64 recompiler against the interpreter (4 configurations x 300 random programs)
    foreach(CFG "nodirected" "nodirected;nocache" "nodirected;core32" "nodirected;link=1")
        string(REPLACE ";" "_" CFGNAME "${CFG}")
        add_test(NAME jit-fuzz-${CFGNAME} COMMAND jit-fuzz-test ${FAKE_ROM} 300 7 ${CFG})
        set_tests_properties(jit-fuzz-${CFGNAME} PROPERTIES TIMEOUT 300)
    endforeach()
    # the directed programs (branches, jumps, memory, arithmetic, immediates, shifts, loops): ~3 minutes
    add_test(NAME jit-fuzz-directed COMMAND jit-fuzz-test ${FAKE_ROM} 1 1 directed)
    set_tests_properties(jit-fuzz-directed PROPERTIES TIMEOUT 900 LABELS slow)
endif()
set_tests_properties(common core rsp instance cli-boot-interpreter cli-boot-recompiler PROPERTIES TIMEOUT 120)
# user data of every test run is isolated from the real user profile
get_property(ALL_TESTS DIRECTORY PROPERTY TESTS)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test-appdata ${CMAKE_BINARY_DIR}/test-localappdata)
foreach(T ${ALL_TESTS})
    set_property(TEST ${T} APPEND PROPERTY ENVIRONMENT "APPDATA=${CMAKE_BINARY_DIR}/test-appdata" "LOCALAPPDATA=${CMAKE_BINARY_DIR}/test-localappdata")
endforeach()

# manual tool (not a ctest test): lists controllers and prints their events
find_package(SDL2 CONFIG QUIET)
if(TARGET SDL2::SDL2)
    add_executable(gamepad-probe EXCLUDE_FROM_ALL gamepad_probe.cpp)
    target_link_libraries(gamepad-probe PRIVATE SDL2::SDL2 SDL2::SDL2main)
    set_target_properties(gamepad-probe PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# ---------------------------------------------------------------------------------------------------------------------
# Tests of the Qt interface (ported from Scripts/linux/run-offscreen-tests.sh). The "offscreen" Qt platform plugin is not part of the
# Qt of vcpkg on Windows, so they run on Qt's "minimal" platform; each group has its own user-data directories.
#   vortex_gui_test(<name> <group> <target> <fixture-role: none|setup|use> [ENV KEY=VALUE...] [ARGS ...])
# ---------------------------------------------------------------------------------------------------------------------
set(VORTEX_PROFILES ${CMAKE_BINARY_DIR}/test-profiles)
function(vortex_gui_group GROUP)
    add_test(NAME gui-${GROUP}-clean COMMAND ${CMAKE_COMMAND} -E remove_directory ${VORTEX_PROFILES}/${GROUP})
    set_tests_properties(gui-${GROUP}-clean PROPERTIES FIXTURES_SETUP gui-${GROUP}-profile)
    add_test(NAME gui-${GROUP}-mkdir COMMAND ${CMAKE_COMMAND} -E make_directory ${VORTEX_PROFILES}/${GROUP}/appdata ${VORTEX_PROFILES}/${GROUP}/localappdata)
    set_tests_properties(gui-${GROUP}-mkdir PROPERTIES FIXTURES_SETUP gui-${GROUP}-profile DEPENDS gui-${GROUP}-clean)
endfunction()
function(vortex_gui_test NAME GROUP TARGET)
    cmake_parse_arguments(T "" "SCALE;TIMEOUT" "ARGS" ${ARGN})
    add_test(NAME gui-${NAME} COMMAND ${TARGET} ${T_ARGS})
    set(ENV_LIST "QT_QPA_PLATFORM=minimal:enable_fonts" "APPDATA=${VORTEX_PROFILES}/${GROUP}/appdata" "LOCALAPPDATA=${VORTEX_PROFILES}/${GROUP}/localappdata")
    if(T_SCALE)
        list(APPEND ENV_LIST "QT_SCALE_FACTOR=${T_SCALE}")
    endif()
    if(NOT T_TIMEOUT)
        set(T_TIMEOUT 180)
    endif()
    set_tests_properties(gui-${NAME} PROPERTIES
        ENVIRONMENT "${ENV_LIST}"
        PASS_REGULAR_EXPRESSION "PASS"
FAIL_REGULAR_EXPRESSION "FAIL;CRASH"
        FIXTURES_REQUIRED gui-${GROUP}-profile
        TIMEOUT ${T_TIMEOUT}
        LABELS gui)
endfunction()

if(TARGET vortex-test AND TARGET language-test)
    foreach(G plugindir language layout vortex plugins presentpolicy)
        vortex_gui_group(${G})
    endforeach()
    vortex_gui_test(plugindir plugindir plugindir-test)
    vortex_gui_test(language language language-test)
    vortex_gui_test(layout layout layout-test)
    foreach(S 1.25 1.5 2)
        vortex_gui_group(layout-${S})
        vortex_gui_test(layout-${S} layout-${S} layout-test SCALE ${S})
    endforeach()
    vortex_gui_test(vortex vortex vortex-test)
    vortex_gui_test(plugins plugins plugins-test)
    vortex_gui_test(present-policy presentpolicy present-policy-test)
    foreach(S 1 1.25 1.5 2)
        vortex_gui_group(cw-${S})
        vortex_gui_test(controller-widget-${S} cw-${S} controller-widget-test SCALE ${S})
    endforeach()
    # the settings pages: write then read must share the user-data directories
    vortex_gui_group(settings)
    vortex_gui_test(settings-write settings settings-test ARGS write)
    vortex_gui_test(settings-read settings settings-test ARGS read)
    vortex_gui_test(settings-rdb settings settings-test ARGS rdb)
    vortex_gui_test(settings-lookup settings settings-test ARGS lookup)
    set_tests_properties(gui-settings-read gui-settings-rdb gui-settings-lookup PROPERTIES DEPENDS gui-settings-write)
    foreach(P video audio input)
        vortex_gui_group(pc-${P})
        if(P STREQUAL "video")
            vortex_gui_test(plugin-config-${P}-prep pc-${P} plugin-config-test ARGS ${P} prep)
        endif()
        vortex_gui_test(plugin-config-${P}-write pc-${P} plugin-config-test ARGS ${P} write)
        vortex_gui_test(plugin-config-${P}-read pc-${P} plugin-config-test ARGS ${P} read)
        set_tests_properties(gui-plugin-config-${P}-read PROPERTIES DEPENDS gui-plugin-config-${P}-write)
        if(P STREQUAL "video")
            set_tests_properties(gui-plugin-config-${P}-write PROPERTIES DEPENDS gui-plugin-config-${P}-prep)
        endif()
    endforeach()
    foreach(V "" "B")
        vortex_gui_group(ic${V})
        vortex_gui_test(input-capture${V}-write ic${V} input-capture-test ARGS write ${V})
        vortex_gui_test(input-capture${V}-read ic${V} input-capture-test ARGS read ${V})
        set_tests_properties(gui-input-capture${V}-read PROPERTIES DEPENDS gui-input-capture${V}-write)
    endforeach()
    foreach(L es en)
        foreach(S 1 1.5)
            vortex_gui_group(iu-${L}-${S})
            vortex_gui_test(input-ui-${L}-${S} iu-${L}-${S} input-ui-test SCALE ${S} ARGS ${L})
        endforeach()
    endforeach()
endif()

# every test executable gets the crash reporter (no error dialogs, a stack on stdout/stderr instead)
foreach(T common-test core-test rsp-test instance-test jit-fuzz-test migration-test first-run-test audio-test lifecycle-test mainwindow-test rombrowser-test mainmenu-test settings-test input-capture-test input-ui-test controller-widget-test
          plugin-config-test present-policy-test debugger-test vortex-test plugindir-test plugins-test language-test layout-test)
    if(TARGET ${T})
        target_sources(${T} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Common/Windows/crash_report_autoinstall.cpp)
    endif()
endforeach()

# ---------------------------------------------------------------------------------------------------------------------
# More ported tests. "window" label: they need a real window (the video plugin embeds its OpenGL window into the main window, which the
# Qt "minimal" platform cannot give), so a window appears for a few seconds; run them with `ctest -L window`, skip with `ctest -LE window`.
# ---------------------------------------------------------------------------------------------------------------------
if(TARGET first-run-test)
    foreach(M new legacy)
        vortex_gui_group(firstrun-${M})
        vortex_gui_test(first-run-${M} firstrun-${M} first-run-test ARGS ${M})
    endforeach()
endif()
if(TARGET migration-test)
    add_test(NAME migration COMMAND migration-test)
    set_tests_properties(migration PROPERTIES PASS_REGULAR_EXPRESSION "PASS" FAIL_REGULAR_EXPRESSION "FAIL;CRASH" TIMEOUT 300)
endif()
if(TARGET audio-test)
    # the plugin's real output through SDL's "disk" audio driver (no sound card): the volume of the settings is applied, mute is silence
    file(MAKE_DIRECTORY ${VORTEX_PROFILES}/templates)
    file(WRITE ${VORTEX_PROFILES}/templates/audio-100.cfg "")
    file(WRITE ${VORTEX_PROFILES}/templates/audio-40.cfg "[Audio-Settings]
Volume=40
")
    file(WRITE ${VORTEX_PROFILES}/templates/audio-mute.cfg "[Audio]
Enable Audio=0
")
    foreach(V 100 40 mute)
        vortex_gui_group(audio-${V})
        add_test(NAME gui-audio-${V}-cfg COMMAND ${CMAKE_COMMAND} -E copy ${VORTEX_PROFILES}/templates/audio-${V}.cfg ${VORTEX_PROFILES}/audio-${V}/appdata/vortex64/Project64.cfg)
        add_test(NAME gui-audio-${V}-dir COMMAND ${CMAKE_COMMAND} -E make_directory ${VORTEX_PROFILES}/audio-${V}/appdata/vortex64)
        set_tests_properties(gui-audio-${V}-dir PROPERTIES FIXTURES_SETUP gui-audio-${V}-cfgdir FIXTURES_REQUIRED gui-audio-${V}-profile)
        set_tests_properties(gui-audio-${V}-cfg PROPERTIES FIXTURES_SETUP gui-audio-${V}-ready FIXTURES_REQUIRED "gui-audio-${V}-profile;gui-audio-${V}-cfgdir")
        add_test(NAME gui-audio-${V} COMMAND ${CMAKE_COMMAND} -E env SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE=${VORTEX_PROFILES}/audio-${V}/out.raw $<TARGET_FILE:audio-test> ${V})
        set_tests_properties(gui-audio-${V} PROPERTIES
            ENVIRONMENT "APPDATA=${VORTEX_PROFILES}/audio-${V}/appdata;LOCALAPPDATA=${VORTEX_PROFILES}/audio-${V}/localappdata"
            PASS_REGULAR_EXPRESSION "PASS" FAIL_REGULAR_EXPRESSION "FAIL;CRASH" FIXTURES_REQUIRED "gui-audio-${V}-ready" TIMEOUT 180 LABELS gui)
    endforeach()
endif()
foreach(W lifecycle-test mainwindow-test)
    if(TARGET ${W})
        vortex_gui_group(${W})
        add_test(NAME window-${W} COMMAND ${W} ${FAKE_ROM})
        set_tests_properties(window-${W} PROPERTIES
            ENVIRONMENT "APPDATA=${VORTEX_PROFILES}/${W}/appdata;LOCALAPPDATA=${VORTEX_PROFILES}/${W}/localappdata"
            PASS_REGULAR_EXPRESSION "PASS" FAIL_REGULAR_EXPRESSION "FAIL;CRASH" FIXTURES_REQUIRED gui-${W}-profile TIMEOUT 600 LABELS window)
    endif()
endforeach()

# ROM browser and main menu (need the Qt Test module: Qt6::Test). The browser test works on a folder that holds "sm64.z64": the header-only fake ROM is copied there.
# (the ROM information dialog is checked against the real Super Mario 64 (USA): name, country, CRCs, CIC chip. A header-only fake ROM cannot satisfy it,
# so the test is registered only when a real ROM is given: cmake -DVORTEX_TEST_ROM=C:/path/to/"Super Mario 64 (USA).z64")
if(TARGET rombrowser-test AND VORTEX_TEST_ROM)
    vortex_gui_group(rombrowser)
    add_test(NAME gui-rombrowser-roms COMMAND ${CMAKE_COMMAND} -E copy ${VORTEX_TEST_ROM} ${VORTEX_PROFILES}/rombrowser/roms/sm64.z64)
    add_test(NAME gui-rombrowser-romsdir COMMAND ${CMAKE_COMMAND} -E make_directory ${VORTEX_PROFILES}/rombrowser/roms)
    set_tests_properties(gui-rombrowser-romsdir PROPERTIES FIXTURES_SETUP gui-rombrowser-romsfolder FIXTURES_REQUIRED gui-rombrowser-profile)
    set_tests_properties(gui-rombrowser-roms PROPERTIES FIXTURES_SETUP gui-rombrowser-ready FIXTURES_REQUIRED "gui-rombrowser-profile;gui-rombrowser-romsfolder")
    vortex_gui_test(rombrowser rombrowser rombrowser-test ARGS ${VORTEX_PROFILES}/rombrowser/roms)
    set_tests_properties(gui-rombrowser PROPERTIES FIXTURES_REQUIRED "gui-rombrowser-profile;gui-rombrowser-ready")
endif()
if(TARGET mainmenu-test)
    vortex_gui_group(mainmenu)
    vortex_gui_test(mainmenu mainmenu mainmenu-test)
endif()
