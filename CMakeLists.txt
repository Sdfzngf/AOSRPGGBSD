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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(AOSRPGGBSD
        src/main.cpp
)

target_compile_definitions(AOSRPGGBSD PRIVATE NDEBUG)

target_sources(AOSRPGGBSD
        PRIVATE
                FILE_SET CXX_MODULES
                FILES src/Basics/Engine.Basics.Memory.cppm
                FILES src/Basics/Engine.Basics.Memory.MemoryStream.cppm
                FILES src/Utils/Engine.Utils.Time.cppm
                FILES src/Utils/Logger/Engine.Utils.Logger.LogLevel.cppm
                FILES src/Utils/Logger/Engine.Utils.Logger.cppm

                FILES src/i18n/Engine.i18n.cppm

                FILES src/Utils/Data/Engine.Utils.Data.DB.cppm
                FILES src/Utils/Data/Engine.Utils.Data.DataEntry.EntryType.cppm
                FILES src/Utils/Data/Engine.Utils.Data.DataEntry.cppm
                FILES src/Utils/Data/Engine.Utils.Data.DataManager.fuction.cppm
                FILES src/Utils/Data/Engine.Utils.Data.DataManager.cppm

                FILES src/Basics/Engine.Basics.Game.cppm
                FILES src/Game/Engine.Game.StartUp.cppm
                FILES src/Game/Engine.Game.ShutDown.cppm
                FILES src/Game/Engine.Game.MainLoop.cppm
)

target_compile_features(AOSRPGGBSD PRIVATE cxx_std_26)
target_link_libraries(AOSRPGGBSD ${LIBS})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
