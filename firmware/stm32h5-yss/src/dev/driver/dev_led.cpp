/*
 * LED 장치 — D11: +3.3V → LED → R32(15K) → PC13. Low 에서 켜진다.
 * 회로도 근거는 firmware/docs/03-board-mapping.md
 */
#include <dev/led.h>
#include <yss/instance.h>

#define LED_PIN     13      // PC13

namespace Led
{
  void initialize(void)
  {
    gpioC.enableClock();
    set(false);                               // 출력으로 바꾸기 전에 끈 레벨을 먼저 둔다
    gpioC.setAsOutput(LED_PIN, Gpio::OSPEED_LOW);
  }

  void set(bool on)
  {
    gpioC.setOutput(LED_PIN, !on);            // active low
  }
}
