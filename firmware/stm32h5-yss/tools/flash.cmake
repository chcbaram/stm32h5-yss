#-------------------------------------------------------------------------------
# probe-rs 기록을 CMake 타겟으로 만든다. 셸 스크립트를 두지 않는 이유는
# Windows / Linux / macOS 에서 같은 명령으로 돌리기 위해서다.
#
#   cmake --build build --target flash
#
# probe-rs 는 STM32H563 타깃과 내장 플래시 기록 알고리즘을 내장하고 있어서
# STM32CubeProgrammer 가 필요 없다. ST-LINK 를 그대로 쓴다.
#-------------------------------------------------------------------------------
find_program(PROBE_RS_EXECUTABLE NAMES probe-rs probe-rs.exe)

set(PROBE_RS_CHIP "STM32H563VITx" CACHE STRING "probe-rs chip name")

if(PROBE_RS_EXECUTABLE)
  add_custom_target(flash
    COMMAND ${PROBE_RS_EXECUTABLE} download
            --chip ${PROBE_RS_CHIP}
            --verify
            $<TARGET_FILE:${EXECUTABLE}>
    COMMAND ${PROBE_RS_EXECUTABLE} reset
            --chip ${PROBE_RS_CHIP}
    DEPENDS ${EXECUTABLE}
    USES_TERMINAL
    COMMENT "probe-rs download (${PROBE_RS_CHIP})"
    )
else()
  add_custom_target(flash
    COMMAND ${CMAKE_COMMAND} -E echo
            "flash 타겟에는 probe-rs 가 필요하다. firmware/docs/10-dev-environment.md 참고."
    COMMAND ${CMAKE_COMMAND} -E false
    )
endif()
