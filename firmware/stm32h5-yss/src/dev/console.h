/*
 * 콘솔 장치 — Led 와 같은 형태 (yss 제작자 예제의 common/dev).
 * 어느 UART 를 쓰는지, 핀, 보레이트는 driver/dev_console.cpp 안에 숨긴다.
 *
 * printf / debug_printf 도 이 콘솔로 나간다 (_write 가 driver/dev_console.cpp 에 있다).
 */
#ifndef DEV_CONSOLE__H_
#define DEV_CONSOLE__H_

#include <stdint.h>

namespace Console
{
  void     initialize(void);

  // 출력. 콘솔 장치를 잠그므로 여러 스레드가 불러도 바이트 단위로는 섞이지 않는다.
  int      write(const void *data, uint32_t len);

  // 입력. 받은 글자가 없으면 -1.
  int16_t  getByte(void);

  // timeout(ms) 안에 글자가 들어오면 true.
  bool     waitUntilReceive(uint32_t timeout);
}

#endif
