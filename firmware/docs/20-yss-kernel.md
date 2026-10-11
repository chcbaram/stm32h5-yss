# 20. yss 커널을 Cortex-M33 에서 돌리기

> STM32H563(Cortex-M33, TrustZone 끔)에서 yss 스케줄러가 돌기까지 고친 것과 막혔던 지점. 결과: 스레드 3개 + Mutex 가 10분 동안 폴트 없이 돈다.
> 관련: [04-yss-structure.md](04-yss-structure.md) · [12-project-skeleton.md](12-project-skeleton.md) · [22-uart-console.md](22-uart-console.md)
>
> 출처: yss `src/scheduler/yss_scheduler.cpp`, `src/system/yss_init.cpp`, `archive/2026.01:src/misc/SEGGER_THUMB_Startup.s` / `startup_stm32h563xx.s` (cmsis-device-h5 v1.7.0) / Armv8-M Architecture Reference Manual B3.19 (EXC_RETURN) / FreeRTOS `portable/GCC/ARM_CM33_NTZ`.

![스택 전환](images/stack-switch.svg)

---

## 1. 한 눈에

고친 것은 다섯 가지다. 앞의 둘은 **보드에서 폴트로 드러났고**, 나머지는 빌드나 코드 검토에서 찾았다.

| # | 문제 | 증상 | 고친 곳 |
|---|---|---|---|
| 1 | main 이 MSP 로 돌고 PSP = 0 | 첫 PendSV 에서 HardFault (정밀 버스 폴트, BFAR `0xFFFF_FFC0`) | H5 msp `switchToProcessStack()` |
| 2 | ST 스타트업의 MSPLIM 이 옛 스택에 남음 | 첫 UART 수신 인터럽트에서 UsageFault STKOF | 같은 함수에서 MSPLIM 도 옮김 |
| 3 | 초기 EXC_RETURN `0xFFFFFFFD` 는 Armv8-M 에서 "Secure 로 복귀" | (Non-secure 코어에서 폴트 예상) | `YSS_INITIAL_EXC_RETURN` = `0xFFFFFFBC` |
| 4 | CM33 분기 누락 | SysTick 이 시작되지 않는다 | `yss_init.cpp`, 스케줄러 타이머, `mcu.h` 매크로 |
| 5 | `YSS__CORE_COUNT` 미정의 | 컴파일 오류 (배열 크기) | `mcu.h` 기본값 1 |

1, 2 는 **SEGGER 스타트업이 해 주던 일**이다. 이전 세션의 M460 CMake 검증은 링크까지만 했기 때문에 드러나지 않았다. GCC 스타트업으로 yss 를 쓰는 모든 타깃에 같은 문제가 있다.

## 2. PSP 전환 — SEGGER 스타트업이 하던 일

yss PendSV 는 `mrs r0, psp` 로 지금 스레드의 문맥을 PSP 아래에 쌓는다. 즉 **Thread 모드가 처음부터 PSP 로 돌고 있어야 한다.** SEGGER 스타트업은 main 전에 이렇게 한다.

```asm
ldr r1, =__stack_process_end__
msr psp, r1
movs r2, #2
msr control, r2          ; SPSEL = 1
```

ST 의 `startup_stm32h563xx.s` 에는 이 단계가 없다. main 이 MSP 로 돌고 PSP 는 리셋값 0 이다. 첫 SysTick → PendSV 에서 `stmdb r0!, {r3-r11}` 이 주소 0 아래에 쓰다가 버스 폴트가 난다.

### 디버거로 본 것

probe-rs 로 폴트 레지스터를 읽고, `probe-rs gdb` + `arm-none-eabi-gdb` 로 스택을 봤다.

| 레지스터 | 값 | 뜻 |
|---|---|---|
| CFSR | `0x0000_8200` | BFARVALID + PRECISERR |
| HFSR | `0x4000_0000` | FORCED (버스 폴트가 HardFault 로 승격) |
| BFAR | `0xFFFF_FFC0` | 0 아래 64 B — FPU 없이 9워드를 쌓으려던 자리 |
| 스택된 PC | `0x0800_11FE` | `PendSV_Handler` + 10 |
| 스택된 xPSR | `0x6900_080E` | 예외 번호 14 = PendSV |
| PSP | `0x0000_0000` | |

### 고친 방법

H5 msp 의 `initializeSystem()` 맨 앞에서 naked 함수 `switchToProcessStack()` 을 부른다.

1. PSP ← 지금의 SP. main 스레드는 **같은 메모리**에서 계속 돈다. 그래서 `SystemInit()` 과 그 호출자의 스택 프레임이 그대로 유효하다.
2. CONTROL.SPSEL = 1, `isb`.
3. MSPLIM ← `__handler_stack_start__` (3절).
4. MSP ← `__handler_stack_end__`. 이제 인터럽트 · 예외만 MSP 로 별도 스택을 쓴다.

새 스택으로 *옮기는* 방법은 쓸 수 없다. `SystemInit()` 안에서 SP 를 다른 곳으로 바꾸면 돌아갈 주소가 옛 스택에 남아 있기 때문이다. 핸들러 스택(2 KB)은 링커 스크립트가 `.handler_stack` 으로 잡는다([12](12-project-skeleton.md) 3절).

## 3. MSPLIM — 두 번째 폴트

1 을 고친 뒤 스레드와 printf 는 돌았다. 그런데 콘솔로 글자를 보내자 멈췄다.

| 레지스터 | 값 | 뜻 |
|---|---|---|
| CFSR | `0x0010_0000` | UsageFault **STKOF** (스택 한계 위반) |

ST 스타트업 첫 줄이 원인이었다.

```asm
Reset_Handler:
  ldr   sp, =_estack
  ldr   r0, =_sstack
  msr   MSPLIM, r0          ; 시작 스택 바닥
```

MSP 를 그보다 낮은 주소의 핸들러 스택으로 옮겼으므로, 핸들러가 MSP 에 무엇이든 쌓으면 한계 아래가 된다. SysTick · TIM2 · PendSV 는 예외 프레임이 PSP(Thread 쪽)에 쌓이고 핸들러가 스택을 거의 쓰지 않아 넘어갔다. USART1 핸들러는 `usart1.isr()` 멤버 함수를 부르면서 MSP 에 push 해서 처음 걸렸다.

전환 함수에서 MSP 를 옮기기 **전에** MSPLIM 을 핸들러 스택 바닥으로 낮춘다. 순서를 바꾸면 잠깐 동안 한계가 SP 보다 위에 있게 된다.

PSPLIM 은 0 으로 둔다. 스케줄러가 스레드마다 PSPLIM 을 바꾸지 않으므로, 값을 넣으면 다른 스레드(힙에 있는 낮은 주소의 스택)로 바뀌는 순간 STKOF 가 난다. 스레드별 스택 한계는 로드맵 23 에서 한다.

## 4. 초기 EXC_RETURN

새 스레드의 첫 진입은 PendSV 가 미리 만들어 둔 가짜 예외 프레임에서 `bx lr` 로 돌아가는 것이다. 이때 LR 값을 yss 는 모든 코어에 `0xFFFFFFFD` 로 넣었다. 그 뒤의 전환은 스레드마다 저장된 실제 LR(r3)을 복원하므로 문제는 **첫 진입뿐**이다.

| 비트 | 6 S | 5 DCRS | 4 FType | 3 Mode | 2 SPSEL | 0 ES |
|---|---|---|---|---|---|---|
| `0xFFFFFFFD` (Armv7-M 값) | 1 Secure | 1 | 1 | 1 | 1 | 1 Secure |
| `0xFFFFFFBC` (Non-secure) | 0 | 1 | 1 | 1 | 1 | 0 |

TrustZone 을 끈(TZEN=0) STM32H5 는 Non-secure 로 돈다. STM32CubeH5 의 비 TrustZone FreeRTOS 예제가 `ARM_CM33_NTZ` 포트(초기 EXC_RETURN `0xFFFFFFBC`)를 쓰는 것과 같다.

```cpp
#if defined(YSS__CORE_CM33_H_GENERIC) && !defined(YSS__CM33_SECURE)
#define YSS_INITIAL_EXC_RETURN  0xFFFFFFBC
#else
#define YSS_INITIAL_EXC_RETURN  0xFFFFFFFD
#endif
```

예전부터 M33 을 지원하던 EFR32BG22 / EFM32PG22 는 Secure 로 돌기 때문에 `mcu.h` 에서 `YSS__CM33_SECURE` 를 정의해 기존 값을 유지한다.

## 5. CM33 분기 채우기

2026.09 는 여러 곳에서 `CM3_CM4_CM7 || CM23` 만 보고 CM33 을 빠뜨렸다.

| 파일 | 빠진 것 | 결과 |
|---|---|---|
| `yss_init.cpp` | `NVIC_SetPriority(PendSV_IRQn, 15)`, `SysTick_Config()` | 스케줄러가 시작되지 않는다 |
| `yss_scheduler.cpp` | `disableSchedulerTimer()` / `enableSchedulerTimer()` | 시그널 대기 중 SysTick 을 멈추지 못한다 |
| `mcu.h` | `__getCoreInterruptStatus` 등 인터럽트 매크로 | 컴파일 오류 |

함께 확인한 것:

- PendSV 우선순위 15: H5 는 우선순위 4비트라 최하위가 맞다.
- `THREAD_GIVEN_CLOCK` 은 시간이 아니라 **코어 클럭 틱 수**다. 250 MHz 에서 250000 = 1 ms 타임슬라이스.
- FPU: msp 가 CPACR 로 CP10/CP11 을 켠다. PendSV 의 `vstmdb {s16-s31}` 에 필요하다.

## 6. 검증

| 항목 | 방법 | 결과 |
|---|---|---|
| 문맥 전환 · delay | 카운터 스레드(1 s) + 에코 스레드 | 카운터 출력, 수신 에코 |
| trigger | 송신 · 수신 때 `trigger::run()` → LED 50 ms | gdb 로 켜짐/꺼짐 시각 확인 ([22](22-uart-console.md) 5절) |
| Mutex | 두 스레드(카운터 · 에코)가 같은 Mutex 로 콘솔을 나눠 씀 | 줄이 섞이지 않는다 |
| 수신 인터럽트 | 콘솔로 `Hi!` 전송 | `rx 'H'` `rx 'i'` `rx '!'` |
| 안정성 | 10분 연속 | 600 초 동안 폴트 없음, CFSR = 0 ([22](22-uart-console.md) 5절) |

## 확인 필요

- 초기 EXC_RETURN 을 `0xFFFFFFFD` 로 두었을 때 실제로 폴트가 나는지는 시험하지 않았다(1 을 고치기 전에는 그 지점까지 가지 못했다). 첫 PendSV 의 LR 을 디버거로 읽어 Non-secure 값인지 확인해 두면 좋다.
- PSP 전환은 H5 msp 에 넣었다. 다른 GCC 타깃(Nuvoton)에도 필요하므로 공통 위치(`msp_default.cpp`)로 옮길지는 원저자와 상의할 일이다.
