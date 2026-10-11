/*
 * 클럭 — HSE 25 MHz → PLL1 → SYSCLK 250 MHz (H563 최대)
 *
 *   PLL1 입력  25 MHz / M(5)   =   5 MHz   (VCO 입력 4~8 MHz 범위)
 *   VCO        5 MHz  x N(100) = 500 MHz   (Wide 192~836 MHz)
 *   P          500    / 2      = 250 MHz → SYSCLK
 *   Q, R       500    / 2      = 250 MHz   (지금은 쓰지 않는다. 출력 비활성)
 *   AHB / APB1 / APB2 / APB3 = 250 MHz (분주 1)
 *
 * 250 MHz 는 VOS0 와 Flash 5 WS + WRHIGHFREQ(프로그래밍 지연) 2 를 쓴다.
 * CubeMX 가 250 MHz 로 만드는 설정과 같다 (stm32h5-w6300 의 SystemClock_Config).
 * 순서: 전압을 먼저 올리고, Wait state 를 늘린 뒤, 클럭을 올린다.
 *
 * yss msp(lib/yss/src/targets/st/msp/msp_stm32h5.cpp)의 weak initializeClock() 을 덮어쓴다.
 * .data/.bss 초기화 전에 불리므로 전역 변수를 쓰지 않는다.
 */
#include <stm32h5xx.h>
#include <stm32h5xx_ll_rcc.h>
#include <stm32h5xx_ll_pwr.h>
#include <stm32h5xx_ll_system.h>


void initializeClock(void)
{
  //-- 1. 전압 스케일 VOS0
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE0);
  while (LL_PWR_IsActiveFlag_VOS() == 0)
  {
  }

  //-- 2. Flash wait state (250 MHz @ VOS0)
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
  MODIFY_REG(FLASH->ACR, FLASH_ACR_WRHIGHFREQ, FLASH_ACR_WRHIGHFREQ_1);
  while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_5)
  {
  }

  //-- 3. HSE (크리스털)
  LL_RCC_HSE_Enable();
  while (LL_RCC_HSE_IsReady() == 0)
  {
  }

  //-- 4. PLL1
  LL_RCC_PLL1_SetSource(LL_RCC_PLL1SOURCE_HSE);
  LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_4_8);
  LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE);
  LL_RCC_PLL1_SetM(5);
  LL_RCC_PLL1_SetN(100);
  LL_RCC_PLL1_SetP(2);
  LL_RCC_PLL1_SetQ(2);
  LL_RCC_PLL1_SetR(2);
  LL_RCC_PLL1P_Enable();
  LL_RCC_PLL1_Enable();
  while (LL_RCC_PLL1_IsReady() == 0)
  {
  }

  //-- 5. 버스 분주 후 SYSCLK 전환
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_1);

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1)
  {
  }
}
