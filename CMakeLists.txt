cmake_minimum_required(VERSION 3.13) # CMP0083 NEW

project(DinguxCommander
  VERSION 0.0.1
  LANGUAGES C CXX)

set(TARGET_PLATFORM host CACHE STRING "Target platform")
set_property(CACHE TARGET_PLATFORM PROPERTY STRINGS host retrofw gcw0 rg350 rs90 rg99)

if(TARGET_PLATFORM STREQUAL "retrofw")
  include(CMake/retrofw_defs.cmake)
elseif(TARGET_PLATFORM STREQUAL "gcw0")
  include(CMake/rg350_defs.cmake)
elseif(TARGET_PLATFORM STREQUAL "rg350")
  include(CMake/rg350_defs.cmake)
elseif(TARGET_PLATFORM STREQUAL "rs90")
  include(CMake/rs90_defs.cmake)
elseif(TARGET_PLATFORM STREQUAL "rg99")
  include(CMake/rg99_defs.cmake)
endif()

option(USE_SDL2 "Use SDL2 instead of SDL1" ON)
option(WITH_SYSTEM_SDL_GFX "Use system SDL_gfx library" OFF)
option(WITH_SYSTEM_SDL_TTF "Use system SDL_ttf library" OFF)

set(RES_DIR \"res/\" CACHE STRING "Resources directory")
set(FILE_SYSTEM \"/dev/mmcblk1p1\" CACHE STRING "Default filesystem (for Disk info)")
set(PATH_DEFAULT "" CACHE STRING "Default path for the left pane (default: getenv(\"PWD\")")
set(PATH_DEFAULT_RIGHT "" CACHE STRING "Default path for the right pane (default: getenv(\"HOME\")")
set(AUTOSCALE 1 CACHE STRING "Auto-detect resolution and DPI scaling factors")
set(WITH_SYSTEM_SDL_TTF OFF CACHE BOOL "Use system sdl_ttf")
set(WITH_SYSTEM_SDL_GFX OFF CACHE BOOL "Use system sdl_gfx")

option(ASAN "Enable address sanitizer" ON)
option(UBSAN "Enable undefined behaviour sanitizer" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clang-tidy

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
  config.cpp
  dialog.cpp
  fileLister.cpp
  fileutils.cpp
  keyboard.cpp
  main.cpp
  panel.cpp
  resourceManager.cpp
  screen.cpp
  sdl_ttf_multifont.cpp
  sdlutils.cpp
  text_edit.cpp
  utf8.cpp
  text_viewer.cpp
  image_viewer.cpp
  window.cpp
)

set(BIN_TARGET commander)
add_executable(${BIN_TARGET} ${DinguxCommander_SRCS})

if (USE_SDL2)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake/sdl2)
  if (WITH_SYSTEM_SDL_TTF)
    find_package(SDL2 REQUIRED)
  else()
    find_package(SDL2 2.0.10 REQUIRED) # for SDL_SIMD_ALIGNED
  endif()
  find_package(SDL2_image REQUIRED)
else()
  find_package(SDL REQUIRED)
  find_package(SDL_image REQUIRED)
endif()

if (WITH_SYSTEM_SDL_GFX)
  if (USE_SDL2)
    find_package(SDL2_gfx REQUIRED)
  else()
    find_library(SDL_gfx SDL_gfx)
    if (NOT SDL_gfx)
      message(SEND_ERROR "Could not find SDL_gfx library.")
    endif()
  endif()
else ()
  # SDL_gfx: rotozoom only.
  if (USE_SDL2)
    add_library(SDL2_gfx STATIC third_party/SDL2_gfx-1.0.4/SDL2_rotozoom.c)
    target_link_libraries(SDL2_gfx PUBLIC SDL2::Core)
    target_include_directories(${BIN_TARGET} PUBLIC third_party/SDL2_gfx-1.0.4/)
    set(SDL2_GFX_LIBRARIES SDL2_gfx)
  else()
    add_library(SDL_gfx STATIC third_party/SDL_gfx-2.0.25/SDL_rotozoom.c)
    target_link_libraries(SDL_gfx PUBLIC ${SDL_LIBRARY})
    target_include_directories(SDL_gfx PRIVATE ${SDL_INCLUDE_DIR})
    target_include_directories(${BIN_TARGET} PUBLIC third_party/SDL_gfx-2.0.25/)
  endif()
endif ()

if (WITH_SYSTEM_SDL_TTF)
  if (USE_SDL2)
    find_package(SDL2_ttf REQUIRED)
    list(APPEND CMAKE_REQUIRED_INCLUDES ${SDL2_TTF_INCLUDE_DIRS})
  else()
    find_library(SDL_ttf SDL_ttf)
    if (NOT SDL_ttf)
      message(SEND_ERROR "Could not find SDL_ttf library.")
    endif (NOT SDL_ttf)
    list(APPEND CMAKE_REQUIRED_INCLUDES ${SDL_INCLUDE_DIR})
  endif()
  include(CheckSymbolExists)
  check_symbol_exists(TTF_OpenFontDPI "SDL_ttf.h" HAS_TTF_OPENFONT_DPI)
else (WITH_SYSTEM_SDL_TTF)
  # SDL_ttf: version from HEAD for independent X/Y DPI scaling.
  find_package(Freetype REQUIRED)
  if (USE_SDL2)
    add_library(SDL2_ttf STATIC third_party/SDL2_ttf/SDL_ttf.c)
    target_link_libraries(SDL2_ttf SDL2::Core ${FREETYPE_LIBRARIES})
    target_include_directories(SDL2_ttf PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_include_directories(${BIN_TARGET} PUBLIC third_party/SDL2_ttf/)
    set(SDL2_TTF_LIBRARIES SDL2_ttf)
  else()
    add_library(SDL_ttf STATIC third_party/SDL_ttf/SDL_ttf.c)
    target_link_libraries(SDL_ttf ${SDL_LIBRARY} ${FREETYPE_LIBRARIES})
    target_include_directories(SDL_ttf PRIVATE ${SDL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS})
    target_include_directories(${BIN_TARGET} PUBLIC third_party/SDL_ttf/)
  endif()
  set(HAS_TTF_OPENFONT_DPI ON)
endif (WITH_SYSTEM_SDL_TTF)

if (HAS_TTF_OPENFONT_DPI)
  target_compile_definitions(${BIN_TARGET} PRIVATE USE_TTF_OPENFONT_DPI)
endif (HAS_TTF_OPENFONT_DPI)

set_target_properties(${BIN_TARGET} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)

if (USE_SDL2)
  target_link_libraries(${BIN_TARGET} PRIVATE
    SDL2::Core
    SDL2::Image
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_GFX_LIBRARIES}
  )
else ()
  target_include_directories(${BIN_TARGET} PUBLIC
    ${SDL_INCLUDE_DIR}
    ${SDL_IMAGE_INCLUDE_DIRS}
  )
  target_link_libraries(${BIN_TARGET} PRIVATE
    ${SDL_LIBRARY}
    ${SDL_IMAGE_LIBRARIES}
    SDL_ttf
    SDL_gfx
  )
endif()
target_link_libraries(${BIN_TARGET} PRIVATE m)

# These variables are defined as C macros if they are tru
foreach(
  def_name
  USE_SDL2
  KEYBOARD_SWAP_SYSTEM_AND_PARENT
)
  if (${def_name})
    list(APPEND def_list ${def_name})
  endif()
endforeach(def_name)


# These variables are passed as C macro definitons if they exist.
foreach(
  def_name
  SCREEN_WIDTH
  SCREEN_HEIGHT
  PPU_X
  PPU_Y
  AUTOSCALE
  RES_DIR
  FONTS
  LOW_DPI_FONTS
  FILE_SYSTEM
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

# These variables are passed as C macro definitons if they're not empty.
foreach(
  def_name
  PATH_DEFAULT
  PATH_DEFAULT_RIGHT
  PATH_DEFAULT_RIGHT_FALLBACK
)
  if(DEFINED ${def_name} AND NOT ${def_name} STREQUAL "")
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

install(
  TARGETS ${BIN_TARGET}
  RUNTIME DESTINATION bin)
install(
  DIRECTORY "${PROJECT_SOURCE_DIR}/res"
  DESTINATION share/commander
  FILES_MATCHING PATTERN "*.png")
install(
  DIRECTORY "${PROJECT_SOURCE_DIR}/res"
  DESTINATION share/commander
  FILES_MATCHING PATTERN "*.ttf")
install(
  FILES "${PROJECT_SOURCE_DIR}/opkg/commander.png"
  DESTINATION share/icons/hicolor/32x32/apps)
