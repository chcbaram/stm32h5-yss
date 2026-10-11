# 10. 개발 환경

> 제조사 IDE(STM32CubeIDE, SEGGER Embedded Studio) 없이 **gcc + CMake + Ninja + probe-rs** 로 빌드, 기록, 디버그한다. 콘솔은 baram-term.
> 관련: [11-sdk-vendoring.md](11-sdk-vendoring.md) · [12-project-skeleton.md](12-project-skeleton.md)

---

## 1. 도구

| 도구 | 검증 버전 | 최소 | 쓰는 곳 |
|---|---|---|---|
| Arm GNU Toolchain | 15.3.1 | 13.2 | 빌드, `arm-none-eabi-gdb` |
| CMake | 4.4.3 | 3.20 | 빌드 |
| Ninja | 1.13.2 | 1.10 | 빌드 |
| probe-rs | 0.32.0 | 0.30 | 기록 (`flash` 타깃), VSCode 디버그 |
| Python | 3.14 | 3.8 | `setup_tools.py`, `fetch_st_sdk.py`, 문서 검사 |
| baram-term | 0.3.0 | — | 콘솔 (외부 제어로 Claude Code 와 같이 쓴다) |

`python3 tools/setup_tools.py` 가 위 도구를 점검하고, 서브모듈과 SVD 를 준비한다. 설치는 직접 하지 않고 OS 별 명령을 알려 준다.

## 2. 왜 probe-rs 인가

| | probe-rs | STM32CubeProgrammer / ST-LINK gdbserver |
|---|---|---|
| 설치 | `brew install probe-rs-tools` | STM32CubeCLT (수 GB, ST 로그인) |
| H563 지원 | 내장 (`STM32H563VITx`) | 내장 |
| VSCode | probe-rs 확장, 또는 cortex-debug + `probe-rs gdb` (main 에서 멈춤) | cortex-debug + gdbserver 경로 지정 |
| 같은 명령으로 Win / Linux / macOS | 예 | 경로가 OS 마다 다름 |

참조 프로젝트(FRDM-IMXRT1186)와 같은 선택이다. 이 PC 에는 STM32CubeCLT 1.22 도 있지만 쓰지 않는다.

## 3. 기록과 디버그

```bash
cd firmware/stm32h5-yss
cmake --build build --target flash      # probe-rs download --verify + reset
```

| 상황 | 명령 |
|---|---|
| 리셋만 | `probe-rs reset --chip STM32H563VITx` |
| 폴트 레지스터 | `probe-rs read --chip STM32H563VITx b32 0xE000ED28 2` (CFSR, HFSR) |
| gdb 로 붙기 | `probe-rs gdb --chip STM32H563VITx` 후 `arm-none-eabi-gdb -ex "target remote :1337" build/stm32h5-yss.elf` |

> ⚠️ `probe-rs gdb` 를 띄워 둔 채로는 `flash` 가 실패한다(프로브를 하나만 잡을 수 있다). 끝나면 종료한다.

VSCode 디버그는 [12](12-project-skeleton.md) 5절.

## 4. 저장소 밖 자산

| 자산 | 위치 | 준비 |
|---|---|---|
| SVD `STM32H563.svd` (15 MB) | `firmware/stm32h5-yss/tools/svd/` (gitignore) | `setup_tools.py` 가 [modm-io/cmsis-svd-stm32](https://github.com/modm-io/cmsis-svd-stm32) 커밋 `e79021a` 에서 받는다 |
| yss | `firmware/stm32h5-yss/lib/yss` (서브모듈) | `git submodule update --init` |

## 확인 필요

- Windows / Linux 에서는 돌려 보지 않았다. 리눅스는 ST-LINK udev 규칙이 필요하다.
