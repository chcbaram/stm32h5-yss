/*
 * bsp — stm32h5-yss 보드 (STM32H563VIT6, 회로도 hardware/stm32h5_kit.pdf)
 *
 *   LED      D11 → PC13, active low                      (src/dev/driver/dev_led.cpp)
 *   콘솔     USART1 TX PA9 / RX PB7 (AF7) → J2 → ST-LINK VCP (src/dev/driver/dev_console.cpp)
 *   HSE      Y1 25 MHz
 *
 * 핀 근거와 실측은 firmware/docs/03-board-mapping.md
 */
#ifndef BSP_H_
#define BSP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>        // printf 는 콘솔로 나간다 (dev_console.cpp 의 _write)

#ifdef __cplusplus
// 보드 장치 (yss 제작자 예제의 bsp.h 와 같다)
#include <dev/led.h>
#include <dev/console.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// yss 를 시작하고(initializeYss) 콘솔과 LED 를 초기화한다. main 에서 가장 먼저 부른다.
void bspInit(void);

#ifdef __cplusplus
}
#endif

#endif
