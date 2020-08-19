cmake_minimum_required(VERSION 3.17)
project(video_test2)

set(CMAKE_C_STANDARD 99)


find_package(SDL2 REQUIRED)
include_directories($ENV{HOME}/ffmpeg_build/include/ ${SDL2_INCLUDE_DIRS})
link_directories($ENV{HOME}/ffmpeg_build/lib/)
add_executable(video_test2 KeyHandler.cpp CFFmpegAudio.cpp CFFmpegVideo.cpp  CMediaPlayer.cpp IMedia.cpp main.cpp)
target_link_libraries(video_test2 avcodec avformat swscale avutil swresample ${SDL2_LIBRARIES} pthread)