cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools")
include(arm-none-eabi-gcc)


# 프로젝트 이름은 최종 출력 파일 이름으로 사용된다.
#
set(PRJ_NAME stm32h5-yss)


project(${PRJ_NAME}
  LANGUAGES ASM C CXX
)


set(EXECUTABLE ${PRJ_NAME}.elf)

set(SRC_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LIB_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(ST_DIR    ${LIB_DIR}/st)


#-------------------------------------------------------------------------------
# CPU 옵션 — yss 라이브러리와 앱이 같은 값을 써야 한다
#-------------------------------------------------------------------------------
set(CPU_FLAGS
  -mcpu=cortex-m33
  -mthumb
  -mfpu=fpv5-sp-d16
  -mfloat-abi=hard
  )


#-------------------------------------------------------------------------------
# 벤더 파일 (tools/fetch_st_sdk.py 가 lib 에 받아 둔 것)
#   yss_vendor 는 yss 가 요구하는 INTERFACE 타깃이다 (lib/yss/CMakeLists.txt).
#-------------------------------------------------------------------------------
add_library(yss_vendor INTERFACE)

target_include_directories(yss_vendor INTERFACE
  ${LIB_DIR}/cmsis/Core/Include
  ${ST_DIR}/device/Include
  ${ST_DIR}/ll/Inc
  )

target_compile_definitions(yss_vendor INTERFACE
  USE_FULL_LL_DRIVER
  HSE_VALUE=25000000              # Y1 25 MHz (docs/03-board-mapping.md)
  )

target_compile_options(yss_vendor INTERFACE
  ${CPU_FLAGS}
  -g3
  -Og
  )

# LL 의 .c 는 쓰는 것만 명시한다. 추가할 때 fetch_st_sdk.py 와 같이 고친다.
#
set(ST_SRC_FILES
  ${ST_DIR}/ll/Src/stm32h5xx_ll_rcc.c
  ${ST_DIR}/ll/Src/stm32h5xx_ll_utils.c
  ${ST_DIR}/device/gcc/startup_stm32h563xx.s
  )


#-------------------------------------------------------------------------------
# yss (서브모듈, chcbaram/yss 의 cmake-h5 브랜치)
#-------------------------------------------------------------------------------
set(YSS_TARGET     stm32h5)
set(YSS_MCU        STM32H563xx)
set(YSS_CONFIG_DIR ${SRC_DIR}/bsp/device)   # yss 가 <config.h> 로 찾는다
set(YSS_HEAP_SIZE  0x20000)       # bsp/ldscript 의 _Min_Heap_Size 와 같게

add_subdirectory(${LIB_DIR}/yss ${CMAKE_BINARY_DIR}/yss)


#-------------------------------------------------------------------------------
# 앱
#-------------------------------------------------------------------------------
file(GLOB SRC_FILES CONFIGURE_DEPENDS
  ${SRC_DIR}/*.cpp
  ${SRC_DIR}/*.c
  )

file(GLOB_RECURSE SRC_FILES_RECURSE CONFIGURE_DEPENDS
  ${SRC_DIR}/ap/*.cpp
  ${SRC_DIR}/ap/*.c
  ${SRC_DIR}/bsp/*.cpp
  ${SRC_DIR}/bsp/*.c
  ${SRC_DIR}/dev/driver/*.cpp
  )

add_executable(${EXECUTABLE}
  ${SRC_FILES}
  ${SRC_FILES_RECURSE}
  ${ST_SRC_FILES}
  )

target_include_directories(${EXECUTABLE} PRIVATE
  ${SRC_DIR}
  ${SRC_DIR}/ap
  ${SRC_DIR}/bsp
  )

target_link_libraries(${EXECUTABLE} PRIVATE yss)

target_compile_options(${EXECUTABLE} PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-threadsafe-statics>

  -fdata-sections
  -ffunction-sections

  -fstack-usage
  -Wmissing-field-initializers
  -Wuninitialized
  -Wall
  )

# 벤더 소스는 프로젝트 코드와 경고 수준을 따로 간다.
#
set_source_files_properties(
  ${ST_SRC_FILES}
  TARGET_DIRECTORY ${EXECUTABLE}
  PROPERTIES
    COMPILE_OPTIONS "-Os;-w"
  )

target_link_options(${EXECUTABLE} PRIVATE
  -T${SRC_DIR}/bsp/ldscript/${PRJ_NAME}.ld

  ${CPU_FLAGS}
  -specs=nano.specs

  -Wl,-Map=${PRJ_NAME}.map,--cref
  -Wl,--gc-sections
  -Wl,--no-warn-rwx-segments
  -Xlinker -print-memory-usage -Xlinker
  )

add_custom_command(TARGET ${EXECUTABLE}
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} ARGS -O binary ${EXECUTABLE} ${PRJ_NAME}.bin
  COMMAND ${CMAKE_OBJCOPY} ARGS -O ihex   ${EXECUTABLE} ${PRJ_NAME}.hex
  COMMENT "Invoking: Make Bin / Hex"
  )


include(flash)
