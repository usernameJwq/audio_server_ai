project(sherpa_cpp)

file(GLOB SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

set(SHERPA_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libs/sherpa-onnx-v2.0.1-linux-x64-shared)

file(GLOB SHERPA_LIBS
    ${SHERPA_INCLUDE_DIR}/*.a
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${SHERPA_INCLUDE_DIR}/include)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC
    audio_base
    fmt::fmt
    ${SHERPA_INCLUDE_DIR}/lib/libsherpa-onnx-c-api.so
    ${SHERPA_INCLUDE_DIR}/lib/libonnxruntime.so
)

add_subdirectory(tests)