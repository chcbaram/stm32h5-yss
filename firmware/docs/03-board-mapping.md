# 03. 보드 결선

> STM32H5_ETH_CAN485 보드(회로도 `hardware/stm32h5_kit.pdf`, Rev 1, 2026-01-08 수정)에서 펌웨어가 쓰는 핀과 부품.
> 관련: [21-clock.md](21-clock.md) · [22-uart-console.md](22-uart-console.md)
>
> 출처: 회로도 7쪽(1 표지, 2 MCU, 3 POWER, 4 PERIPHERAL, 5 CAN_LIN, 6 RS485_RS232, 7 ETHERNET). 칩 확인은 probe-rs 로 읽은 DBGMCU_IDCODE.

---

## 1. 한 눈에

| 항목 | 값 | 근거 |
|---|---|---|
| MCU | **STM32H563VIT6** (LQFP100, Flash 2 MB, SRAM 640 KB) | 회로도 2쪽 U1. `DBGMCU_IDCODE = 0x1007_6484` (DEV_ID 0x484 = H56x/H57x) |
| HSE | **25 MHz** 크리스털 Y1 (PH0/PH1), 부하 20 pF | 2쪽 |
| LSE | 32.768 kHz Y3 (PC14/PC15), 12 pF | 2쪽 |
| 디버그 | J2 (6핀): 3.3V · SWDIO · SWCLK · **USART1 TX/RX** · GND | 2쪽 Debug Conn |
| 리셋 | S2 → NRST | 2쪽 |
| BOOT0 | S1 누르면 High, 평소 10K 로 Low | 2쪽 |
| 전원 | DC 잭 VIN → SY8205FC(5V) → AMS1117-3.3 | 3쪽 |

## 2. 펌웨어가 쓰는 핀

| 기능 | 핀 | 설정 | 근거 |
|---|---|---|---|
| LED (D11) | **PC13** | 출력. **Low 에서 켜짐** | 4쪽 LED: +3.3V → D11 → R32 15K → `LED` 넷 → PC13 |
| 콘솔 TX | **PA9** | USART1_TX, AF7 | 2쪽 `U1_TXD` (68번 핀) → J2 |
| 콘솔 RX | **PB7** | USART1_RX, AF7 | 2쪽 `U1_RXD` (93번 핀) → J2 |
| SWD | PA13 / PA14 | 리셋 기본값 | 2쪽 `SWDIO` / `SWCLK` → J2 |

PC13 은 백업 도메인 핀이라 싱크 전류가 3 mA 로 제한된다. R32 가 15K 라 0.1 mA 정도만 흐른다. 문제 없다.

AF 번호(AF7)는 STM32H563 데이터시트의 대체 기능 표 값이다. **PA9 TX 와 PB7 RX 모두 보드에서 송수신으로 확인했다**([22](22-uart-console.md)).

## 3. 디버거 연결

J2 에 ST-LINK V2-1 을 물린다. SWD 와 USART1 이 같은 커넥터에 있어서 케이블 하나로 기록, 디버그, 콘솔을 다 한다. 콘솔은 ST-LINK 의 가상 COM 포트(VCP)로 나온다.

| PC 에서 보이는 것 | 값 (이 PC 기준) |
|---|---|
| probe-rs | `STLink V2-1 -- 0483:3752:0673FF525784864967204317` |
| 시리얼 | `/dev/cu.usbmodem1412302` · USB 설명 `STM32 STLink` |

시리얼 경로는 PC 와 USB 자리마다 다르다. baram-term 에서는 `--match STLink` 로 고른다([22](22-uart-console.md) 4절).

## 4. 아직 쓰지 않는 주변장치

로드맵 후반에 쓸 것들이다. 오른쪽 열은 **회로도 2쪽의 넷 이름**을 그대로 옮긴 것이다. 넷 이름의 UART 번호(U4 등)가 실제 주변장치 번호와 같은지, 그 핀에 해당 AF 가 있는지는 구현할 때 데이터시트로 확인한다.

| 블록 | 부품 (회로도 쪽) | 넷 → 핀 |
|---|---|---|
| Ethernet | LAN8742A U16, 25 MHz Y2 (7쪽) | REF_CLK PA1 · MDIO PA2 · CRS_DV PA7 · MDC PC1 · RXD0 PC4 · RXD1 PC5 · TX_EN PA5 · TXD0 PB12 · TXD1 PB15 · NRST PC2 |
| CAN-FD ×2 | TJA1051T/3 U3 · U4 (5쪽) | CAN1 RXD PD0 / TXD PD1 · CAN2 RXD PB5 / TXD PB6 |
| RS485 1 | SP3485EEN U13 (6쪽) | U5_TXD PB13 · U5_RXD PD2 · U5_DE PC8 |
| RS485 2 / LIN 3 | U2 / TJA1021 U12, J21 로 선택 (4 · 5 · 6쪽) | U4_TXD PA0 · U4_RXD PD11 · U4_DE PB14 |
| LIN 1 · 2 | TJA1021 U7 · U11 (5쪽) | U2_TXD PD5 / U2_RXD PD6 · U3_TXD PB10 / U3_RXD PD9 |
| RS232 | U6 (6쪽) | U7_TXD PE8 · U7_RXD PE7 |
| UART 헤더 | J16 · J17 (6쪽) | U10_TXD PE3 / U10_RXD PE2 · U6_TXD PC6 / U6_RXD PC7 |
| SPI Flash | W25Q256JVEIQ U10 (4쪽) | SPI1 NSS PA4 · MISO PA6 · SCK PB3 · MOSI PD7 |
| microSD | J1 (4쪽) | SDMMC1 D0 PA10 · D1~D3, CLK PC9~PC12 · CMD PB2 · DET PB4 |
| FRAM | FM24C16B U5, 2 KB (4쪽) | I2C4 SCL PD12 / SDA PD13 |
| OLED | J18 (4쪽) | I2C1 SCL PB8 / SDA PB9 |
| 부저 | Q5 (4쪽) | BUZZER_PWM PA8 |
| 버튼 | S5 (4쪽) | SW PC0 |
| USB-C | J7 (4쪽) | USB_DM PA11 · USB_DP PA12 |
| 헤더 | J19 30핀 (2쪽) | PE0 · PE4~PE6 · PE9~PE15 · PC3 · PD3 · PD4 · PD8 · PD10 · PD14 · PD15 · PA3 · PA15 · PB0 · PB1 |

## 확인 필요

- 4절은 넷 이름 기준이다. SDMMC1 의 D1~D3 · CLK 가 PC9~PC12 중 어디에 붙는지는 회로도 글자가 잘려 확인하지 못했다.
- HSE 실제 주파수는 MCO 로 재지 않았다. `SystemCoreClock` 계산값과 UART 보레이트가 맞는 것으로 간접 확인했다([21](21-clock.md)).
