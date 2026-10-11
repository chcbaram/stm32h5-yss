/*
 * 콘솔 장치 — USART1, TX PA9 / RX PB7 (AF7) → J2 → ST-LINK VCP, 115200 8N1.
 * 회로도 근거는 firmware/docs/03-board-mapping.md
 */
#include <dev/console.h>
#include <yss/instance.h>
#include <yss/debug.h>
#include <stdarg.h>
#include <stdio.h>

#define CONSOLE         usart1
#define CONSOLE_BAUD    115200

namespace Console
{
  void initialize(void)
  {
    gpioA.enableClock();
    gpioB.enableClock();
    gpioA.setAsAltFunc(9, Gpio::AF7);     // USART1_TX
    gpioB.setAsAltFunc(7, Gpio::AF7);     // USART1_RX

    Uart::config_t config =
    {
      Uart::MODE_NORMAL,  // mode
      CONSOLE_BAUD,       // baudrate
      Uart::STOP_1BIT,    // stopbit
      Uart::PARITY_NONE,  // parity
      false,              // hwfc
      nullptr,            // rcvBuf (힙에서 할당)
      256                 // rcvBufSize
    };

    CONSOLE.enableClock();
    CONSOLE.initialize(config);
    CONSOLE.enableInterrupt();
  }

  int write(const void *data, uint32_t len)
  {
    CONSOLE.lock();
    CONSOLE.send((void *)data, len);
    CONSOLE.unlock();
    return len;
  }

  int16_t getByte(void)
  {
    return CONSOLE.getRxByte();
  }

  bool waitUntilReceive(uint32_t timeout)
  {
    return CONSOLE.waitUntilReceive(timeout);
  }
}

// newlib: printf → _write → 콘솔
// <errno.h> 는 넣지 않는다. newlib 의 error_t 와 yss 의 error_t 가 충돌한다.
extern "C" int _write(int file, char *ptr, int len)
{
  if (file == 1 || file == 2)
    return Console::write(ptr, len);

  return -1;
}

// SEGGER 런타임이 주던 함수. yss 가 오류 메시지에 쓴다(yss/debug.h, C++ 링크).
int32_t debug_printf(const char *fmt, ...)
{
  va_list args;
  int     ret;

  va_start(args, fmt);
  ret = vprintf(fmt, args);
  va_end(args);
  return ret;
}
