# stm32h5-yss

STM32H563 커스텀 보드(STM32H5_ETH_CAN485)에서 [yss OS](https://github.com/mymy49/yss) 를 돌리는 펌웨어.

원저자는 SEGGER Embedded Studio 로 빌드한다. 이 저장소는 yss 의 커널과 API 는 그대로 두고, **gcc + CMake + Ninja + probe-rs** 로 빌드하며, STM32H5 타깃을 STM32 LL 로 새로 썼다.

| | |
|---|---|
| MCU | STM32H563VIT6 (Cortex-M33 250 MHz, Flash 2 MB, SRAM 640 KB) · TrustZone 끔 |
| 디버거 | ST-LINK V2-1 (J2: SWD + USART1 콘솔) → probe-rs |
| 툴체인 | Arm GNU Toolchain 15.3 · CMake 4.4 · Ninja 1.13 |
| yss | 서브모듈, [chcbaram/yss](https://github.com/chcbaram/yss) `cmake-h5` 브랜치 |
| 벤더 | cmsis-device-h5 · stm32h5xx-hal-driver(LL 만) v1.7.0, CMSIS 6.4.0 — 필요한 파일만 커밋 고정 |
| 상태 | yss 스레드 · Mutex · Period · UART 송수신 · 250 MHz 동작 확인 |

## 구성

```
hardware/             회로도 (stm32h5_kit.pdf)
tools/
  setup_tools.py      새 PC 점검 + 서브모듈 + SVD
firmware/
  docs/               보드 · yss 구조 · 환경 · 구현 기록 (한국어)
  stm32h5-yss/        펌웨어 (CMake)
    lib/              외부 코드: yss (서브모듈), st, cmsis
    src/              앱과 bsp
```

## 준비

```bash
git clone --recursive <이 저장소>
python3 tools/setup_tools.py
```

## 빌드와 기록

```bash
cd firmware/stm32h5-yss
cmake -S . -B build -G Ninja
cmake --build build
cmake --build build --target flash
```

콘솔은 ST-LINK 가상 COM 포트, 115200 8N1. VSCode 는 `firmware/stm32h5-yss/prj/stm32h5-yss.code-workspace` 를 연다.

## 문서

[firmware/docs/README.md](firmware/docs/README.md) 에서 시작한다. 현재 상태, 로드맵, 목차가 있다.

| 대역 | 내용 |
|---|---|
| 00~09 | 보드 결선 · yss 구조와 이력 |
| 10~19 | 개발 환경 · 벤더 파일 · 프로젝트 구조 |
| 20~ | 기능별 구현 기록 (커널 이식, 클럭, 콘솔 …) |

## 검사

```bash
python3 firmware/docs/check_svg.py
python3 firmware/docs/check_links.py
python3 firmware/stm32h5-yss/tools/fetch_st_sdk.py --check
```

## 라이선스

[LICENSE](LICENSE) (MIT). `firmware/stm32h5-yss/lib/` 아래는 각 원본의 라이선스(yss MIT, ST BSD-3-Clause, Arm Apache-2.0)를 따른다.
