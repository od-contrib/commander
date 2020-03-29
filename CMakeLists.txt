cmake_minimum_required(VERSION 3.10) # CMP0083 NEW

project(DinguxCommander
  VERSION 0.0.1
  LANGUAGES C CXX)

option(ASAN "Enable address sanitizer" ON)
option(UBSAN "Enable undefined behaviour sanitizer" ON)

if(CMAKE_BUILD_TYPE MATCHES "Release")

  # Enable link-time optimization if supported.
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported OUTPUT ipo_check_output)
  if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(WARNING "LTO is not supported: ${ipo_check_output}\n")
  endif()

  set(ASAN OFF)
  set(UBSAN OFF)
endif()

set(DinguxCommander_SRCS
  commander.cpp
  dialog.cpp
  fileLister.cpp
  fileutils.cpp
  keyboard.cpp
  main.cpp
  panel.cpp
  resourceManager.cpp
  screen.cpp
  sdlutils.cpp
  viewer.cpp
  window.cpp
)

set(BIN_TARGET commander)
add_executable(${BIN_TARGET} ${DinguxCommander_SRCS})

find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)

option(WITH_SYSTEM_SDL_GFX "Use system SDL_gfx library" OFF)

if (WITH_SYSTEM_SDL_GFX)
	find_library(SDL_gfx SDL_gfx)
	if (NOT SDL_gfx)
		message(SEND_ERROR "Could not find SDL_gfx library.")
	endif ()
else ()
	# SDL_gfx: rotozoom only.
	add_library(SDL_gfx STATIC third_party/SDL_gfx-2.0.25/SDL_rotozoom.c)
	target_link_libraries(SDL_gfx PUBLIC ${SDL_LIBRARY})
	target_include_directories(SDL_gfx PRIVATE ${SDL_INCLUDE_DIR})

	target_include_directories(${BIN_TARGET} PUBLIC third_party/SDL_gfx-2.0.25/)
endif ()

# SDL_ttf: version from HEAD for independent X/Y DPI scaling.
add_library(SDL_ttf STATIC third_party/SDL_ttf/SDL_ttf.c)
find_package(Freetype REQUIRED)
target_include_directories(SDL_ttf PRIVATE ${SDL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(SDL_ttf ${SDL_LIBRARY} ${FREETYPE_LIBRARIES})

set_target_properties(${BIN_TARGET} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)

target_include_directories(${BIN_TARGET} PUBLIC
  third_party/SDL_ttf/
  ${SDL_INCLUDE_DIR}
  ${SDL_IMAGE_INCLUDE_DIRS}
)
target_link_libraries(${BIN_TARGET} PRIVATE
  ${SDL_LIBRARY}
  ${SDL_IMAGE_LIBRARIES}
  SDL_ttf
  SDL_gfx
  m
)

if(TARGET_PLATFORM STREQUAL "retrofw")
  include(CMake/retrofw_defs.cmake)
elseif(TARGET_PLATFORM STREQUAL "gcw0")
  include(CMake/rg350_defs.cmake)
elseif(TARGET_PLATFORM STREQUAL "rg350")
  include(CMake/rg350_defs.cmake)
elseif(TARGET_PLATFORM STREQUAL "rs90")
  include(CMake/rs90_defs.cmake)
endif()

foreach(
  def_name
  SCREEN_WIDTH
  SCREEN_HEIGHT
  PPU_X
  PPU_Y
  AUTOSCALE
  RES_DIR
  PATH_DEFAULT
  PATH_DEFAULT_RIGHT
  PATH_DEFAULT_RIGHT_FALLBACK
  FILE_SYSTEM
  FONT_PATH
  MYKEY_UP
  MYKEY_RIGHT
  MYKEY_DOWN
  MYKEY_LEFT
  MYKEY_SYSTEM
  MYKEY_PAGEUP
  MYKEY_PAGEDOWN
  MYKEY_OPEN
  MYKEY_PARENT
  MYKEY_OPERATION
  MYKEY_SELECT
  MYKEY_TRANSFER
)
  if(DEFINED ${def_name})
    list(APPEND def_list ${def_name}=${${def_name}})
  endif()
endforeach(def_name)
target_compile_definitions(${BIN_TARGET} PRIVATE ${def_list})

if(ASAN)
target_compile_options(${BIN_TARGET} PUBLIC -fsanitize=address -fsanitize-recover=address)
target_link_libraries(${BIN_TARGET} PUBLIC -fsanitize=address -fsanitize-recover=address)
endif()

if(UBSAN)
target_compile_options(${BIN_TARGET} PUBLIC -fsanitize=undefined)
target_link_libraries(${BIN_TARGET} PUBLIC -fsanitize=undefined)
endif()
