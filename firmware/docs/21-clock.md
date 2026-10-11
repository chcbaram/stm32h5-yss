# 21. 클럭 — HSE 25 MHz → 250 MHz

> SYSCLK 을 H563 최대인 250 MHz 로 올리고, runtime 타이머(TIM2)를 1 MHz 로 돌린다. 결과: `SystemCoreClock 250000000 Hz`, 115200 / 921600 bps 송수신 정상.
> 관련: [03-board-mapping.md](03-board-mapping.md) · [20-yss-kernel.md](20-yss-kernel.md)
>
> 출처: `src/bsp/device/clock.cpp` / stm32h5xx-hal-driver v1.7.0 LL 헤더 / CubeMX 가 250 MHz 로 만든 `SystemClock_Config()` (stm32h5-w6300 `src/bsp/bsp.c`) 의 순서와 값.

---

## 1. 클럭 트리

| 단계 | 설정 | 값 |
|---|---|---|
| 입력 | HSE 크리스털 Y1 | 25 MHz |
| PLL1 M | ÷ 5 | 5 MHz (VCO 입력 범위 4~8 MHz) |
| PLL1 N | × 100 | VCO 500 MHz (Wide 192~836 MHz) |
| PLL1 P | ÷ 2 | **250 MHz → SYSCLK** |
| PLL1 Q · R | ÷ 2 | 250 MHz, 지금은 출력을 켜지 않는다 |
| AHB · APB1 · APB2 · APB3 | ÷ 1 | 250 MHz |
| APB1 타이머 | APB1 분주가 1 이라 × 1 | 250 MHz → TIM2 PSC 249 → 1 MHz |
| USART1 커널 클럭 | 리셋 기본값 PCLK2 | 250 MHz |

## 2. 순서

`device/clock.cpp` 의 `initializeClock()` 이 yss msp 의 weak 함수를 덮어쓴다. 순서는 **전압 → 대기 상태 → 클럭** 이다. 거꾸로 하면 클럭이 올라간 순간 Flash 를 못 읽는다.

1. `LL_PWR_SetRegulVoltageScaling(VOS0)` → VOSRDY 대기
2. Flash LATENCY 5 WS, WRHIGHFREQ 2 — CubeMX 의 `FLASH_LATENCY_5` + `FLASH_PROGRAMMING_DELAY_2` 와 같다
3. HSE 켜기 → 준비 대기
4. PLL1 설정 (위 표) → P 출력 켜기 → PLL1 켜기 → 준비 대기
5. 버스 분주 ÷1, SYSCLK = PLL1 → 전환 확인

그 뒤 msp 가 ICACHE 를 켠다.

## 3. SystemCoreClock 이 덮어쓰이는 문제

ST 스타트업은 `.data` 를 복사하기 **전에** `SystemInit()` 을 부른다. 그래서 그 안에서 `SystemCoreClock` 에 250 MHz 를 써도 곧이어 `.data` 초기값(32 MHz)으로 덮인다. 그래서 두 가지를 지킨다.

- `initializeClock()` 은 전역 변수와 드라이버 객체를 쓰지 않는다. LL(레지스터)만 쓴다.
- `initializeYss()` 가 맨 먼저 부르는 `initializeSystemTime()` 이 `SystemCoreClockUpdate()` 로 다시 계산한다. RCC 레지스터에서 읽으므로 정확하다.

`system_stm32h5xx.c` 는 빌드에 넣지 않는다. yss msp 가 `SystemInit()` 을 제공해 중복 정의가 되기 때문이다. 그 파일에 있던 `AHBPrescTable` / `APBPrescTable` 은 LL 이 쓰므로 msp 에 옮겨 두었다.

## 4. runtime 타이머

`thread::delay()` 와 `Period` 는 `runtime::getUsec()` 를 폴링한다. TIM2(32비트)를 1 MHz 로 돌리고 오버플로 인터럽트로 상위 32비트를 센다. 오버플로는 2³² µs = 71.6 분마다 한 번이다.

### 막혔던 점 — 시작부터 71분

처음 올렸을 때 첫 출력이 `count 0, 4294972 ms` 였다. 2³²/1000 + 5 ms 다. PSC 를 바로 적용하려고 UG(갱신 이벤트)를 걸면 UIF 가 서고, 그게 오버플로로 한 번 세어졌다. URS = 1 (오버플로에서만 UIF)로 바꿔 해결했다.

`getUsec()` 는 인터럽트를 막고 상위 값과 CNT 를 읽는다. 그 사이에 넘쳤는데 인터럽트가 아직 안 돌았으면(UIF 가 서 있고 CNT 가 작으면) 상위 값에 1 을 더한다.

디버거가 코어를 멈추면 TIM2 도 멈춘다(`DBGMCU_APB1FZR1.DBG_TIM2_STOP`). 브레이크포인트에서 오래 서 있다 풀어도 delay 가 한꺼번에 끝나지 않는다.

## 확인 필요

- 지연 설정(5 WS, WRHIGHFREQ 2)은 CubeMX 결과를 따랐다. RM0481 의 표로 직접 확인하지는 않았다.
- HSE 와 PLL 출력을 MCO 핀으로 재지 않았다. 보레이트가 맞고, 보드 시간과 PC 시간의 차이가 작은 것([22](22-uart-console.md) 5절)으로 간접 확인했다.
- 250 MHz 에서 전류와 발열은 재지 않았다.
