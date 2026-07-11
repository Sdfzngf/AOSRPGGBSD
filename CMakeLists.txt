cmake_minimum_required(VERSION 3.28.0)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_GENERATOR "Ninja")
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O3")
project(AOSRPGGBSD VERSION 0.1.0 LANGUAGES C CXX)

# Bootstrap CPM.cmake
set(CPM_DOWNLOAD_VERSION 0.40.7)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
        file(DOWNLOAD
                https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
                ${CPM_DOWNLOAD_LOCATION}
        )
endif()
include(${CPM_DOWNLOAD_LOCATION})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SDL_SHARED OFF CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL static library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)

CPMAddPackage(
        NAME SDL3
        GITHUB_REPOSITORY libsdl-org/SDL
        GIT_TAG release-3.4.10
        OPTIONS
                "CMAKE_EXPORT_COMPILE_COMMANDS OFF"
                "SDL_SHARED OFF"
                "SDL_STATIC ON"
                "SDL_TESTS OFF"
)

if(NOT TARGET SDL3::SDL3-static)
        message(FATAL_ERROR "SDL3 static target SDL3::SDL3-static is unavailable")
endif()

CPMAddPackage(
        NAME replxx
        GITHUB_REPOSITORY AmokHuginnsson/replxx
        GIT_TAG release-0.0.4
        OPTIONS
                "CMAKE_EXPORT_COMPILE_COMMANDS OFF"
                "BUILD_SHARED_LIBS OFF"
)

if(NOT TARGET replxx)
        message(FATAL_ERROR "replxx target is unavailable")
endif()

add_executable(AOSRPGGBSD
        src/main.cpp
)

add_custom_target(PackRes
        COMMAND "rm" -rf ./bin/Game
        COMMAND "mkdir" ./bin/Game/
        COMMAND $<TARGET_FILE:AOSRPGGBSD> pack ../src/Resources/test.rres ./bin/Game/b.dat 11451
        USES_TERMINAL
        VERBATIM
)

add_custom_target(BGM
        COMMAND "lmms" render ../src/Resources/BGM/FallInMyDream.mmpz -o ../src/Resources/BGM/FallInMyDream.wav
        COMMAND "lmms" render ../src/Resources/BGM/ITERATION.mmpz -o ../src/Resources/BGM/ITERATION.wav
        USES_TERMINAL
        VERBATIM
)

#target_compile_definitions(AOSRPGGBSD PRIVATE NDEBUG)

target_sources(AOSRPGGBSD
        PRIVATE
                FILE_SET CXX_MODULES
                FILES src/Basics/Engine.Basics.Memory.cppm
                FILES src/Basics/Engine.Basics.Memory.MemoryStream.cppm
                FILES src/Basics/Engine.Basics.TextUtils.cppm
                FILES src/Basics/Engine.Basics.TextUtils.functions.cppm
                FILES src/Utils/Engine.Utils.Time.cppm
                FILES src/Utils/Logger/Engine.Utils.Logger.LogLevel.cppm
                FILES src/Utils/Logger/Engine.Utils.Logger.cppm

                FILES src/i18n/Engine.i18n.cppm

                FILES src/Utils/Arguments/Engine.Utils.Arg.Actions.cppm
                FILES src/Utils/Arguments/Engine.Utils.Arg.MArg.cppm
                FILES src/Utils/Arguments/Engine.Utils.Arg.Format.cppm

                FILES src/GUI/LDRM/Engine.GUI.LDRM.cppm
                FILES src/GUI/OpenCL/Engine.GUI.OpenCL.cppm
                FILES src/GUI/SDL/Engine.GUI.SDL.cppm
                FILES src/GUI/Engine.GUI.GUIManager.cppm

                FILES src/Utils/DevConsole/Engine.Utils.DevConsole.MainAct.cppm
                FILES src/Utils/DevConsole/Engine.Utils.DevConsole.cppm

                FILES src/Utils/Data/Engine.Utils.Data.DB.cppm
                FILES src/Utils/Data/Engine.Utils.Data.DataEntry.EntryType.cppm
                FILES src/Utils/Data/Engine.Utils.Data.DataEntry.cppm
                FILES src/Utils/Data/Engine.Utils.Data.DataManager.functions.cppm
                FILES src/Utils/Data/Engine.Utils.Data.DataManager.cppm

                FILES src/Basics/Engine.Basics.Game.cppm
                FILES src/Game/Engine.Game.StartUp.cppm
                FILES src/Game/Engine.Game.ShutDown.cppm
                FILES src/Game/Engine.Game.MainLoop.cppm
)

target_compile_features(AOSRPGGBSD PRIVATE cxx_std_26)
#target_link_libraries(AOSRPGGBSD ${LIBS})
target_link_libraries(AOSRPGGBSD PRIVATE SDL3::SDL3-static replxx)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
