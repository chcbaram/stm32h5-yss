/*
 * yss 설정 — stm32h5-yss (STM32H563VIT6)
 *
 * 원본 템플릿: lib/yss/config/config_stm32h5xx.h
 */
#ifndef YSS_CONFIG__H_
#define YSS_CONFIG__H_

// runtime 에 쓸 32비트 타이머 (TIM2 또는 TIM5). 1 MHz 로 돌린다.
#define YSS_RUNTIME_TIMER   RUNTIME_TIM2

// 스레드 타임슬라이스 = 코어 클럭 틱 수. 250 MHz / 250000 = 1 ms.
#define THREAD_GIVEN_CLOCK  250000

// 최대 스레드 수 (main 포함)
#define MAX_THREAD          8

// 스레드 스택을 0xAA 로 채워 사용량을 볼 수 있게 한다
#define FILL_THREAD_STACK   true

#define USE_GUI             false
#define NUM_OF_YSS_KEY      0

// 주변장치
#define USART1_ENABLE       true    // 디버그 콘솔: PA9 TX / PB7 RX → J2 → ST-LINK VCP
#define USART2_ENABLE       false
#define USART3_ENABLE       false
#define UART4_ENABLE        false
#define UART5_ENABLE        false
#define USART6_ENABLE       false
#define UART7_ENABLE        false

#endif
