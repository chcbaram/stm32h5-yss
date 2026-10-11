# 04. yss 구조와 이력

> yss OS 가 어떻게 짜여 있고, 왜 STM32 코드가 없으며, 이 프로젝트가 어느 브랜치를 바탕으로 하는지.
> 관련: [12-project-skeleton.md](12-project-skeleton.md) · [20-yss-kernel.md](20-yss-kernel.md)
>
> 출처: [mymy49/yss](https://github.com/mymy49/yss) 소스와 git 기록, 포크 [chcbaram/yss](https://github.com/chcbaram/yss). 이전 세션의 인수인계 문서(Claude Docs "yss OS × STM32H5 커스텀 보드 CMake 프로젝트 인수인계").

---

## 1. 한 눈에

| 항목 | 내용 |
|---|---|
| 언어 · 라이선스 | C++11, MIT (Copyright 2015 Yoon-Ki Hong) |
| 원저자 빌드 | SEGGER Embedded Studio (`.hzp`). Makefile · CMake 없음 |
| 스케줄러 | 우선순위 없는 라운드로빈. SysTick 이 PendSV 를 걸고 PendSV 가 문맥을 바꾼다 |
| 지연 | `thread::delay()` 는 runtime 타이머를 폴링하며 `yield()` 를 반복한다. 주기 실행은 `Period` (6절) |
| 동기화 | `Mutex` (티켓 락 + yield), `Trigger`. 세마포어 · 큐는 없다 |
| 이 프로젝트의 바탕 | **`Release/2026.09`** → 포크의 `cmake-h5` 브랜치 |

## 2. 계층

드라이버는 세 층이다. H5 타깃은 아래 두 층만 새로 썼다.

| 층 | 위치 | 하는 일 |
|---|---|---|
| 인터페이스 | `inc/drv/Uart.h` 등 | 공통 기반 클래스. 링버퍼 같은 공통 동작과 순수 가상 함수(`changeBaudrate`, `send`) |
| 드라이버 | `src/targets/<vendor>/driver/drv_*.cpp` | 레지스터 조작. H5 는 LL 로 쓴다 |
| 인스턴스 | `src/targets/<vendor>/instance/instance_*.cpp` | 클럭 · IRQ · 리셋 함수와 전역 객체(`usart1` 등), IRQ 핸들러 |

`Gpio.h`, `Clock.h`, `Dma.h` 는 MCU 매크로로 벤더 헤더를 고르는 선택자다. 지원하지 않는 드라이버는 헤더가 `YSS_DRV_<NAME>_UNSUPPORTED` 를 정의해 빠진다.

## 3. MCU 선택 흐름

1. 컴파일 옵션의 품번 매크로 (`-DSTM32H563xx`)
2. `inc/drv/mcu.h` 가 계열 매크로(`STM32H5`)와 코어 매크로(`YSS__CORE_CM33_H_GENERIC`)를 정의
3. `inc/drv/peripheral.h` 가 벤더 디바이스 헤더(`stm32h5xx.h`)를 include
4. `inc/yss/instance.h` 가 벤더 인스턴스 헤더(`targets/st/instance.h`)를 include

앱은 `config.h` 를 제공한다(`MAX_THREAD`, `THREAD_GIVEN_CLOCK`, `YSS_RUNTIME_TIMER`, 주변장치별 `*_ENABLE`). 템플릿은 `config/config_<mcu>.h`.

## 4. STM32 코드가 없는 이유

2026.03 릴리스에서 ST, GigaDevice, SiLabs, WIZnet, NXP, Maxim 타깃이 삭제됐다(368개 파일, 약 30만 줄, 커밋 메시지 "당장 못쓰거나 안쓰는 파일은 일단 제거"). 다른 저장소로 옮긴 것은 아니다. H5 는 어느 시점에도 지원된 적이 없다. 가장 가까운 예전 코드는 G4 드라이버다.

옛 기록은 포크에 브랜치로 고정했다. 원래 기록은 브랜치에서 끊긴 객체라 언제든 사라질 수 있다.

| 브랜치 (포크, 로컬) | 가리키는 것 | 내용 |
|---|---|---|
| `archive/legacy-2025-01` | `576898a` (2025-01-01) | 원래 개발 기록 1,418 커밋. 저자 템플릿 STM32F446 이 서브모듈로 고정한 해시 |
| `archive/2026.01` | `6c71a60^` | 삭제 직전 스냅샷. `src/targets/st` 에 F0/F1/F4/F7/G4 파일 84개 |

> ⚠️ 두 브랜치는 아직 **로컬에만** 있다. 포크에 push 해야 백업이 된다.

## 5. 2026.09 에서 달라진 것 (인수인계 문서 이후)

인수인계 문서는 2026.08 기준이었다. 포크에는 그 뒤의 `Release/2026.09` 가 있었고, 이 브랜치를 바탕으로 했다.

| 항목 | 2026.08 | 2026.09 |
|---|---|---|
| 스케줄러 | `yss_scheduler.cpp` 약 880줄 | 1,364줄. 멀티코어 지원을 단일 코어 스케줄러에 합쳤다 (`semaphore::lockSchedule()` 등) |
| 초기 EXC_RETURN 위치 | 6곳 (`thread::add` 4, `trigger::run` 2) | **2곳** (스택 프레임을 만드는 함수 하나씩) |
| `YSS__CORE_COUNT` | 없음 | 배열 크기로 쓴다. 단일 코어 타깃 대부분이 정의하지 않는다 → 기본값 1 을 넣었다 |
| CM33 분기 | 일부 | SysTick 시작, 스케줄러 타이머 켜기/끄기, 인터럽트 매크로에서 CM33 이 빠져 있다 → 채웠다 ([20](20-yss-kernel.md)) |

## 6. 주기 실행 — `Period`

FreeRTOS 의 `vTaskDelayUntil` 에 해당하는 것은 `util/Period.h` 의 `Period` 다. 기준 시각을 들고 있다가 `wait()` 가 "기준 + 주기"까지 양보하며 기다린다. 그래서 루프 몸체에 걸린 시간만큼 주기가 밀리지 않는다.

```cpp
Period period(1000000);   // us
period.reset();           // 기준 시각을 지금으로. 빠뜨리면 기준이 0 이라 처음에 연달아 돌아온다
while (1)
{
  work();
  period.wait();
}
```

`thread::delay(1000)` 로 돌리면 몸체 시간이 더해져 주기가 1,001.5 ms 가 되고 누적됐다. `Period` 로 바꾸자 회차마다 0~3 µs 만 흔들리고 누적되지 않았다([22](22-uart-console.md) 5절).

## 확인 필요

- `Release/2026.09` 가 원저자의 "릴리스" 인지 진행 중 브랜치인지는 확인하지 않았다. upstream `main` 은 아직 2026.08 이다.
- 2026.09 의 Nuvoton M4xx 빌드는 `YSS__CORE_COUNT` 가 없어 원래 깨져 있었을 것으로 본다. 직접 빌드해 보지는 않았다.
