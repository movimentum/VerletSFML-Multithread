cmake_minimum_required(VERSION 3.16)
project(Verlet-Multithread LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x)
FetchContent_MakeAvailable(SFML)

file(GLOB_RECURSE source_files
    "src/*.cpp"
)

set(SOURCES ${source_files})

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "src" "engine")
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics)

# FFMPEG includes and libs
target_include_directories(${PROJECT_NAME} PRIVATE "$ENV{FFMPEG_DEV}/include")
target_link_directories(${PROJECT_NAME} PRIVATE "$ENV{FFMPEG_DEV}/lib")
target_link_libraries(${PROJECT_NAME} PRIVATE avcodec avdevice avfilter avformat avutil swresample swscale)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# Copy res dir to the binary directory
add_custom_command(
    TARGET ${PROJECT_NAME}
    COMMENT "Copy Res directory"
    PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:${PROJECT_NAME}>/res
    VERBATIM)

if(WIN32)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        COMMENT "Copy OpenAL DLL"
        PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
endif()


# Copy all ffmpeg dll to binary directory
file(GLOB FFMPEG_DLL "$ENV{FFMPEG_DEV}/bin/*.dll")
add_custom_command(
        TARGET ${PROJECT_NAME}
        COMMENT "Copy FFMPEG DLL"
        #PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy $ENV{FFMPEG_DEV}/bin/*.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
		PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${FFMPEG_DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)