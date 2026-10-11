/*
 * stm32h5-yss 링커 스크립트 — STM32H563VIT6 (Flash 2 MB, SRAM 640 KB)
 *
 * cmsis-device-h5 의 STM32H563xx_FLASH.ld (lib/st/device/gcc/*.ref) 를 바탕으로 했다.
 * 바꾼 점:
 *   - 힙을 고정 크기 영역으로 잡는다(.heap). yss 는 스레드 스택을 new 로 힙에서 받으므로
 *     힙이 넉넉해야 한다. 크기는 CMakeLists.txt 의 YSS_HEAP_SIZE 와 같아야 한다
 *     (yss 의 __HEAP_SIZE__ 로 넘어간다).
 *   - _sbrk(src/bsp/syscalls.c) 가 _sheap ~ _eheap 를 넘지 않게 경계 심볼을 둔다.
 *   - 스택이 둘이다.
 *       main 스택 : RAM 끝(_estack). 스타트업이 여기서 시작하고, yss msp 가 main 전에
 *                   Thread 모드를 PSP 로 바꾸면서 이 자리를 그대로 main 스레드 스택으로 쓴다.
 *       핸들러 스택: .handler_stack (__handler_stack_end__). 인터럽트와 예외만 MSP 로 쓴다.
 *     다른 스레드는 각자 힙에서 받은 스택을 PSP 로 쓴다 (docs/20-yss-kernel.md).
 *
 * TrustZone 은 끈 상태(TZEN=0)이므로 non-secure 별칭(0x0800_0000, 0x2000_0000)을 쓴다.
 */
ENTRY(Reset_Handler)

_Min_Heap_Size  = 0x20000;   /* 128 KB — CMakeLists.txt 의 YSS_HEAP_SIZE 와 같게 */
_Min_Stack_Size = 0x1000;    /*   4 KB — main 스레드 (PSP) */
_Handler_Stack_Size = 0x800; /*   2 KB — 인터럽트 / 예외 (MSP) */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 640K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);
_sstack = _estack - _Min_Stack_Size;

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >FLASH

  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  . = ALIGN(4);
  .bss (NOLOAD) :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  .heap (NOLOAD) :
  {
    . = ALIGN(8);
    _sheap = .;
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = ALIGN(8);
    _eheap = .;
  } >RAM

  .handler_stack (NOLOAD) :
  {
    . = ALIGN(8);
    __handler_stack_start__ = .;
    . = . + _Handler_Stack_Size;
    . = ALIGN(8);
    __handler_stack_end__ = .;
  } >RAM

  /* main 스택 자리가 남는지 확인만 한다. 넘치면 링크가 실패한다. */
  .stack_check (NOLOAD) :
  {
    . = ALIGN(8);
    ASSERT(. <= _sstack, "RAM overflow: .bss + .heap + .handler_stack overlaps the main stack");
  } >RAM

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
