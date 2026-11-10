project(tts_sherpa)

file(GLOB SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

set(SHERPA_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libs/sherpa-onnx-v2.0.1-linux-x64-static-lib/lib)

file(GLOB SHERPA_LIBS
    ${SHERPA_INCLUDE_DIR}/*.a
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC ${SHERPA_LIBS})