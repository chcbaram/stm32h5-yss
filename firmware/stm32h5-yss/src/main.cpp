/*
 * stm32h5-yss — yss OS 를 STM32H563 커스텀 보드에서 돌린다.
 *
 * 스레드 2개와 트리거 1개가 돈다. 콘솔을 쓰는 두 스레드는 같은 Mutex 를 잡는다.
 *   - countThread    : 1 초 주기(Period)로 카운터를 콘솔(USART1)에 출력하고 LED 트리거를 건다
 *   - echoThread     : 콘솔로 받은 글자를 "rx 'a'" 형태로 돌려 주고 LED 트리거를 건다
 *   - trigger_blinkLed: LED 를 50 ms 켰다 끈다 (송신 · 수신 표시, yss 제작자 예제와 같은 방식)
 * LED 깜빡임과 콘솔 출력으로 컨텍스트 전환, delay, Mutex, trigger 를 한 번에 확인한다.
 */
#include <yss.h>
#include <util/runtime.h>
#include <util/Period.h>
#include <bsp.h>

static Mutex       gPrintMutex;
static triggerId_t gLedTrigger;

// 트리거는 trigger::run() 으로 깨울 때마다 처음부터 한 번 돌고 끝난다.
// 이미 도는 중에 run() 이 오면 무시되므로, 연달아 받아도 50 ms 한 번만 켜진다.
static void trigger_blinkLed(void)
{
  Led::set(true);
  thread::delay(50);
  Led::set(false);
}

// Period 는 FreeRTOS 의 vTaskDelayUntil 과 같다. 기준 시각 + 주기까지 기다리므로
// printf 에 걸린 시간만큼 주기가 밀리지 않는다. 루프 전에 reset() 으로 기준을 잡아야 한다.
static void countThread(void)
{
  uint32_t count = 0;
  Period   period(1000000);     // us

  period.reset();
  while (1)
  {
    gPrintMutex.lock();
    printf("count %lu, %lu ms\n", count++, (uint32_t)runtime::getMsec());
    gPrintMutex.unlock();
    trigger::run(gLedTrigger);
    period.wait();
  }
}

static void echoThread(void)
{
  while (1)
  {
    if (Console::waitUntilReceive(1000) == false)
      continue;

    int16_t c;
    while ((c = Console::getByte()) >= 0)
    {
      gPrintMutex.lock();
      printf("rx '%c' 0x%02X\n", c >= 0x20 && c < 0x7F ? c : '.', c);
      gPrintMutex.unlock();
    }
    trigger::run(gLedTrigger);
  }
}

int main(void)
{
  bspInit();     // yss 시작(initializeYss) + 콘솔 · LED

  printf("\nstm32h5-yss %s %s\n", __DATE__, __TIME__);
  printf("SystemCoreClock %lu Hz\n", SystemCoreClock);

  gLedTrigger = trigger::add(trigger_blinkLed, 1024);
  thread::add(countThread, 2048);
  thread::add(echoThread, 2048);

  while (1)
  {
    thread::yield();
  }
}
