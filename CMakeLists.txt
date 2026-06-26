cmake_minimum_required(VERSION 3.21)

project(SRTForge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_add_executable(SRTForge
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/ModelManager.cpp
    src/ModelManager.h
    src/OllamaCorrector.cpp
    src/OllamaCorrector.h
    src/SystemMemory.cpp
    src/SystemMemory.h
    src/TranscriptionRunner.cpp
    src/TranscriptionRunner.h
)

target_link_libraries(SRTForge PRIVATE Qt6::Widgets Qt6::Network)

if(APPLE)
    set(PROMPTS_OUTPUT_DIR "$<TARGET_BUNDLE_CONTENT_DIR:SRTForge>/Resources/prompts")
else()
    set(PROMPTS_OUTPUT_DIR "$<TARGET_FILE_DIR:SRTForge>/prompts")
endif()

add_custom_command(TARGET SRTForge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROMPTS_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/prompts/correction_prompt.txt"
        "${PROMPTS_OUTPUT_DIR}/correction_prompt.txt"
)

add_custom_target(copy_prompts ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROMPTS_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/prompts/correction_prompt.txt"
        "${PROMPTS_OUTPUT_DIR}/correction_prompt.txt"
    DEPENDS "${CMAKE_SOURCE_DIR}/prompts/correction_prompt.txt"
)
add_dependencies(copy_prompts SRTForge)

if(APPLE)
    set_target_properties(SRTForge PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "SRTForge"
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.srtforge.app"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()
