/*
 * bsp — yss 시작과 보드 장치 초기화. yss 제작자 예제의 initializeBoard() 와 같은 역할.
 */
#include "bsp.h"
#include <yss.h>


void bspInit(void)
{
  //-- yss 시작: runtime 타이머, SysTick(스케줄러), PendSV 우선순위.
  //   Console::initialize() 가 new(힙 잠금 → 스케줄러)를 쓰고, 콘솔 출력이
  //   Mutex 를 쓰므로 맨 먼저 해야 한다.
  initializeYss();

  Console::initialize();
  Led::initialize();
}
