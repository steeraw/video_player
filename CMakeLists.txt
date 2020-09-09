cmake_minimum_required(VERSION 3.17)
project(video_test3)

set(CMAKE_CXX_STANDARD 20)


find_package(SDL2 REQUIRED)
include_directories($ENV{HOME}/ffmpeg_build/include/ ${SDL2_INCLUDE_DIRS})
link_directories($ENV{HOME}/ffmpeg_build/lib/)
add_executable(video_test3 KeyHandler.cpp CFFmpegVideo.cpp  CMediaPlayer.cpp IMedia.cpp CConfig.cpp main.cpp)
target_link_libraries(video_test3 avcodec avformat swscale avutil swresample ${SDL2_LIBRARIES} pthread)