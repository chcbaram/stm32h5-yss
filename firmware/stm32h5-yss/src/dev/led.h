/*
 * LED 장치 — yss 제작자 예제(common/dev/led.h)와 같은 형태.
 * 핀과 극성은 driver/dev_led.cpp 안에 숨기고, 앱은 켜고 끄기만 한다.
 */
#ifndef DEV_LED__H_
#define DEV_LED__H_

namespace Led
{
  void initialize(void);
  void set(bool on);
}

#endif
