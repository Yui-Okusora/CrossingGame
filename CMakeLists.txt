# CMakeList.txt : CMake project for CrossingGame, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("CrossingGame")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/glad)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/gl2d)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/stb_image)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/stb_truetype)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/miniaudio-master)

# Add source to this project's executable.
add_executable (CrossingGame
    "src/CrossingGame.cpp"
    "src/Engine/Core/Application.cpp"
    "src/Engine/Core/Processor.cpp"
    "src/Engine/Core/Layer.cpp"
    "src/Engine/Context/EngineContext.cpp"

    # Hardware IO Subsystems
    "src/Engine/System/AudioEngine.cpp"
    "src/Engine/System/EventSystem.cpp"
    "src/Engine/System/Window.cpp"

    # Graphics Rendering Pipeline
    "src/Engine/Graphics/AssetManager.cpp"
    

    # Physics & Spatial Queries
    "src/Engine/Physics/CollisionEngine.cpp"

    # Immediate-Mode Interaction UI
    
    "src/Engine/UI/UIContext.cpp"

    # Mathematical Utilities
    "src/Engine/Utils/CRC32_64/CRC32_64.cpp"
  "src/Engine/Graphics/RenderStream.cpp" "src/Game/Layers/MainMenu.cpp" "src/Game/Layers/Gameplay.cpp" "src/Game/Layers/HUD.cpp" "src/Game/Layers/PauseMenu.cpp" "src/Game/Layers/SaveLoadLayer.cpp" "src/Game/Layers/SettingsLayer.cpp" "src/Game/Layers/WinLoseLayer.cpp"  "src/Game/Data/GameStateData.cpp" "src/Game/Entities/GoalEntity.cpp" "src/Game/Entities/BenchEntity.cpp" "src/Game/Entities/BusEntity.cpp" "src/Game/Entities/ElevatorCrowdEntity.cpp" "src/Game/Entities/CodeStreamEntity.cpp" "src/Game/Entities/LogPlatformEntity.cpp" "src/Game/Entities/StudentPlayerEntity.cpp" "src/Game/Entities/TeacherNPCEntity.cpp")

target_include_directories(CrossingGame PRIVATE include/ src/)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CrossingGame PROPERTY CXX_STANDARD 20)
endif()

if (NOT MSVC)
    target_compile_options(CrossingGame PRIVATE -O3)
endif()

target_compile_definitions(CrossingGame PUBLIC RESOURCES_PATH="${CMAKE_SOURCE_DIR}/resources/")

target_link_libraries(CrossingGame
    PRIVATE
        glad
        glfw
        gl2d
        stb_image
        stb_truetype
        miniaudio
)

add_custom_command(TARGET CrossingGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:CrossingGame>/resources"
)
