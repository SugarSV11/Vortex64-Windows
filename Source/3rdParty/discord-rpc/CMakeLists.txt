# Rich presence client of Discord (the vendored discord-rpc library): the connection to the Discord
# client is the Unix socket of connection_unix.cpp, the Windows counterpart being connection_win.cpp.
if(WIN32)
    set(DISCORD_PLATFORM_SRC connection_win.cpp discord_register_win.cpp)
else()
    set(DISCORD_PLATFORM_SRC connection_unix.cpp discord_register_linux.cpp)
endif()
add_library(discord_rpc STATIC
    discord_rpc.cpp
    rpc_connection.cpp
    serialization.cpp
    ${DISCORD_PLATFORM_SRC}
)
target_include_directories(discord_rpc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(discord_rpc PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON)
# the vendored rapidjson (2016) has a template body that GCC 14 and later reject without -fpermissive
if(MSVC)
    target_compile_options(discord_rpc PRIVATE /w)
    target_compile_definitions(discord_rpc PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(discord_rpc PUBLIC advapi32 shell32)
else()
    target_compile_options(discord_rpc PRIVATE -w -fpermissive)
    target_link_libraries(discord_rpc PUBLIC Threads::Threads)
endif()
