# 11. 벤더 파일 가져오기

> STM32CubeH5 를 통째로 받지 않고, 쓰는 파일 30개만 커밋 SHA 를 고정해 `lib/` 에 넣었다. 평소 빌드에는 네트워크가 필요 없다.
> 관련: [10-dev-environment.md](10-dev-environment.md) · [12-project-skeleton.md](12-project-skeleton.md)
>
> 출처: `firmware/stm32h5-yss/tools/fetch_st_sdk.py`, `lib/st/SOURCES.md`.

---

## 1. 무엇을 어디서

| 저장소 | ref | 받은 것 | 저장 위치 |
|---|---|---|---|
| STMicroelectronics/cmsis-device-h5 | v1.7.0 = `884b8dc7` | `stm32h5xx.h`, `stm32h563xx.h`, `system_stm32h5xx.h`, `partition_stm32h5xx.h`, `startup_stm32h563xx.s`, 링커 스크립트(참고용 `.ref`) | `lib/st/device/` |
| STMicroelectronics/stm32h5xx-hal-driver | v1.7.0 = `4b270332` | LL 헤더 10개 (bus, cortex, gpio, icache, pwr, rcc, system, tim, usart, utils), `ll_rcc.c`, `ll_utils.c` | `lib/st/ll/` |
| ARM-software/CMSIS_6 | v6.4.0 | Core 헤더 (`core_cm33.h` 와 m-profile) | `lib/cmsis/` |

태그 대신 태그가 가리키는 **커밋**을 적었다. 태그는 옮겨질 수 있다. 각 파일의 sha256 앞 12자는 `lib/st/SOURCES.md` 에 남는다.

```bash
python3 tools/fetch_st_sdk.py --check    # 받아 둔 파일이 목록과 같은지
python3 tools/fetch_st_sdk.py            # 다시 받기 (버전 올릴 때)
```

## 2. 받지 않은 것

| 파일 | 이유 |
|---|---|
| `system_stm32h5xx.c` | yss msp 가 `SystemInit()` / `SystemCoreClockUpdate()` 를 제공해 중복 정의가 난다. 그 안의 `AHBPrescTable` / `APBPrescTable` 만 msp 로 옮겼다 |
| HAL (`stm32h5xx_hal_*.c`) | 드라이버는 LL 로만 쓴다. HAL 은 `HAL_Init`, SysTick 소유권, 콜백 구조가 yss 와 겹친다 |
| LL `.c` 대부분 | LL 은 대부분 `static inline` 헤더다. 구조체 기반 `LL_xxx_Init()` 은 쓰지 않는다 |

LL 함수를 하나 더 쓰다가 `undefined reference` 가 나면, 그 `.c` 를 `fetch_st_sdk.py` 의 `LL_SOURCES` 와 `CMakeLists.txt` 의 `ST_SRC_FILES` 에 같이 추가한다.

## 3. 라이선스

| 묶음 | 라이선스 |
|---|---|
| cmsis-device-h5, stm32h5xx-hal-driver | BSD-3-Clause (각 폴더의 `LICENSE.md`) |
| CMSIS 6 | Apache-2.0 (`lib/cmsis/LICENSE`) |
| yss | MIT |

BSD-3 와 Apache-2.0 은 MIT 프로젝트와 함께 쓸 수 있다. 원본의 저작권 표시를 지우지 않는다.
