cmake_minimum_required(VERSION 3.10) # CMP0083 NEW

project(DinguxCommander
  VERSION 0.0.1
  LANGUAGES C CXX)

option(ASAN "Enable address sanitizer" ON)
option(UBSAN "Enable undefined behaviour sanitizer" ON)

if(CMAKE_BUILD_TYPE MATCHES "Release")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(ASAN OFF)
  set(UBSAN OFF)
endif()

set(DinguxCommander_SRCS
  third_party/SDL_ttf/SDL_ttf.c
  commander.cpp
  dialog.cpp
  fileLister.cpp
  fileutils.cpp
  keyboard.cpp
  main.cpp
  panel.cpp
  resourceManager.cpp
  sdlutils.cpp
  viewer.cpp
  window.cpp
)

set(BIN_TARGET commander)
add_executable(${BIN_TARGET} ${DinguxCommander_SRCS})

find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)
find_package(Freetype REQUIRED)
target_include_directories(${BIN_TARGET} PUBLIC
  third_party/SDL_ttf/
  ${SDL_INCLUDE_DIR}
  ${SDL_IMAGE_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
)
target_link_libraries(${BIN_TARGET} PRIVATE
  ${SDL_LIBRARY}
  ${SDL_IMAGE_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  m
)

foreach(
  def_name
  PPU_Y
  RES_DIR
)
  if(DEFINED ${def_name})
    list(APPEND def_list ${def_name}=${${def_name}})
  endif()
endforeach(def_name)
target_compile_definitions(${BIN_TARGET} PRIVATE ${def_list})

if(RETROFW)
  target_compile_definitions(${BIN_TARGET} PRIVATE PLATFORM_DINGOO)
  set_target_properties(${BIN_TARGET} PROPERTIES OUTPUT_NAME "commander.dge")
endif()

if(ASAN)
target_compile_options(${BIN_TARGET} PUBLIC -fsanitize=address -fsanitize-recover=address)
target_link_libraries(${BIN_TARGET} PUBLIC -fsanitize=address -fsanitize-recover=address)
endif()

if(UBSAN)
target_compile_options(${BIN_TARGET} PUBLIC -fsanitize=undefined)
target_link_libraries(${BIN_TARGET} PUBLIC -fsanitize=undefined)
endif()
