/* Memories definition */
MEMORY
{
  RAM    (xrw)   : ORIGIN = 0x20000000,   LENGTH = 96K
  FLASH   (rx)   : ORIGIN = 0x08000000,   LENGTH = 512K
}

/* Core stack definition at the absolute top of the 96KB RAM */
_estack = 0x20000000 + 96K; /* 0x20018000 (8-byte aligned) */

_Min_Heap_Size = 0;
_Min_Stack_Size = 0x400; /* 1KB Main system stack */

SECTIONS
{
  /* The startup vector table goes first in Flash */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* The program code (.text) and read-only data (.rodata) */
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

  /* Used by the startup assembly to copy initialized globals from FLASH to RAM */
  _sidata = LOADADDR(.data);

  /* Initialized globals are placed in SRAM */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        
    *(.data)           
    *(.data*)          
    . = ALIGN(4);
    _edata = .;        
  } >RAM AT> FLASH

  /* Uninitialized variables (.bss) are zeroed at bootup */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;         
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;         
    __bss_end__ = _ebss;
  } >RAM

  /* Crucial: exported for ThreadX to find the first unused RAM address for its memory pool */
  __RAM_segment_used_end__ = ALIGN(8);
}
