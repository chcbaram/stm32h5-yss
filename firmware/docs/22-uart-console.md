# 22. GPIO · USART 드라이버와 콘솔

> yss 의 `Gpio` / `Uart` 를 STM32 LL 로 구현하고 USART1 을 printf 콘솔로 쓴다. 결과: 115200 bps 출력과 수신, `changeBaudrate(921600)`, 두 스레드가 Mutex 로 나눠 써도 줄이 섞이지 않는다.
> 관련: [03-board-mapping.md](03-board-mapping.md) · [20-yss-kernel.md](20-yss-kernel.md) · [21-clock.md](21-clock.md)
>
> 출처: `lib/yss/src/targets/st/driver/drv_{gpio,uart}_stm32h5.cpp`, `instance_{gpio,uart}_stm32h5.cpp` / RM0481 USART 장 / 예전 yss STM32 드라이버(`archive/2026.01`).

---

## 1. GPIO

API 는 예전 yss STM32 드라이버(`class_gpio_stm32.h`)와 같게 했다. AF 는 데이터시트 표의 번호를 그대로 쓴다.

```cpp
gpioC.enableClock();
gpioC.setAsOutput(13, Gpio::OSPEED_LOW);
gpioA.setAsAltFunc(9, Gpio::AF7);     // USART1_TX
```

MODER, OTYPER, OSPEEDR, PUPDR, AFR 은 한 포트의 핀들이 같이 쓰는 레지스터라 read-modify-write 다. 두 스레드가 같은 포트의 다른 핀을 동시에 설정하면 서로 덮을 수 있어서, 설정 함수는 인터럽트를 잠깐 막는다. 출력은 BSRR 이라 원자적이다.

## 2. USART

| 항목 | 구현 |
|---|---|
| 수신 | RXNE 인터럽트 → `Uart::push()` 로 링버퍼 (`Uart` 기반 클래스) |
| 송신 | 폴링. TXE 가 빌 때까지 `thread::yield()` 하므로 다른 스레드는 계속 돈다 |
| 오류 | ORE · FE · NE · PE 는 ICR 로 지우고, RDR 은 항상 읽어 RXNE 를 내린다 |
| 패리티 | NONE · ODD · EVEN. 패리티가 있으면 9비트 프레임 (데이터 8비트 유지). MARK · SPACE 는 미지원 |
| FIFO · DMA | 쓰지 않는다. DMA(GPDMA)는 로드맵 25 |

BRR 은 UE = 0 일 때만 쓸 수 있다. `changeBaudrate()` 는 마지막 프레임이 나갈 때까지(TC) 기다린 뒤 끄고, 바꾸고, 다시 켠다.

인스턴스는 `config.h` 의 `USART1_ENABLE` 등으로 골라 컴파일한다. 매크로 하나가 클럭 · NVIC · 리셋 함수, 객체, IRQ 핸들러를 같이 만든다.

## 3. printf 경로

```
printf → newlib → _write (src/dev/driver/dev_console.cpp) → Console::write → usart1.lock / send / unlock
```

콘솔은 LED 처럼 장치 모듈(`namespace Console`, `src/dev/console.h`)이다. 어느 UART 를 쓰는지와 핀은 `dev_console.cpp` 안에만 있고, 앱은 `Console::write` / `getByte` / `waitUntilReceive` 와 printf 만 쓴다. `dev_console.cpp` 는 `<errno.h>` 를 넣지 않는다. newlib 의 `typedef int error_t` 가 yss 의 `enum error_t` 와 충돌한다.

`Console::write` 는 `usart1` 의 Mutex 를 잡는다. 그래서 바이트 단위로는 섞이지 않는다. 다만 printf 한 번이 여러 번의 `_write` 로 나뉠 수 있으므로 **줄 단위로 섞이지 않게 하려면 앱이 자기 Mutex 로 printf 를 감싼다** (`main.cpp` 의 `gPrintMutex`).

yss 가 오류 메시지에 쓰는 `debug_printf()` 는 SEGGER 런타임 함수다(제작자 예제는 이것을 IDE 디버그 터미널로 본다). `yss/debug.h` 가 C++ 링크로 선언하므로 `dev_console.cpp` 에 두고 콘솔로 내보낸다(C 파일에 두면 링크 이름이 달라 못 찾는다).

## 4. baram-term 으로 보기

콘솔은 ST-LINK VCP 로 나온다. baram-term 창에서 `STM32 STLink` 포트를 115200 으로 열고 **포트 메뉴 > 외부 제어 허용** 을 켜면 Claude Code 가 같은 창을 통해 보내고 읽는다.

```bash
baram-ctl --match STLink status
baram-ctl --match STLink send "Hi!" --eol none --until "rx '!'" --timeout 5
```

리셋 직후 부팅 메시지를 보려면 리셋 전에 mark 를 잡는다.

```bash
M=$(baram-ctl --match STLink --json status | python3 -c "import json,sys;print(json.load(sys.stdin)['mark'])")
probe-rs reset --chip STM32H563VITx
baram-ctl --match STLink read --since $M
```

다른 보레이트 시험처럼 포트를 직접 열어야 할 때는 `baram-ctl --match STLink release` 로 비우고, 끝나면 `resume` 한다.

## 5. 검증 결과

```
stm32h5-yss Oct 11 2026 12:17:09
SystemCoreClock 250000000 Hz
count 0, 5 ms
count 1, 1006 ms
count 2, 2008 ms
rx 'H' 0x48
rx 'i' 0x69
rx '!' 0x21
count 3, 3009 ms
```

| 항목 | 결과 |
|---|---|
| 115200 송신 · 수신 | 정상. 수신 3바이트 모두 에코 |
| 921600 (`changeBaudrate`) | 정상. 전환 뒤 출력과 수신 `rx 'Z'` 확인 (`usart1` 커널 클럭 250 MHz) |
| `thread::delay(1000)` 주기 | 1,001.5 ms. 몸체(printf 약 1.5 ms)가 더해져 **누적된다** (`1006 → 2008 → 3009`) |
| `Period(1000000)` 주기 | 63회 µs 측정: 간격 999,997 ~ 1,000,003 µs, 기준 대비 0 ~ +3 µs, **누적 없음**. 125회 ms 측정에서도 모든 간격 1,000 ms |
| LED 트리거 | gdb 로 `Led::set` 추적: 1 초마다 켜지고 정확히 50.0 ms 뒤 꺼진다 (TIM2 42,007,208 → 42,057,209 µs). 수신(`abc`)에도 트리거가 걸리고 폴트 없음 |
| 10분 연속 (`delay` 버전) | count 3 → 602, 600 초 동안 폴트 없음 (끝난 뒤 CFSR = 0) |

`Period` 의 남은 오차는 회차마다 깨어나는 지연(지터) 3 µs 이내다. 그보다 큰 오차는 HSE 크리스털 자체의 ppm 오차뿐인데, PC 시각과 비교하는 방법은 해상도가 ±800 ppm 정도라 재지 못했다(확인 필요).

## 확인 필요

- 송신을 폴링으로 하므로 긴 출력 중에는 그 스레드가 타임슬라이스를 다 쓴다. 출력량이 늘면 DMA(로드맵 25)로 바꾼다.
- HSE 크리스털의 실제 오차(ppm). 주파수 카운터로 MCO 를 재거나, 수십 분 이상 PC 와 비교해야 한다.
- 하드웨어 흐름 제어(`hwfc`)는 코드만 있고 시험하지 않았다. 이 보드의 J2 에는 RTS/CTS 가 없다.
