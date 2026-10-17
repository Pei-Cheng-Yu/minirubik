ENTRY(main)

SECTIONS
{
    . = 0;
    .text : { *(.text*) }

    . = 0x10000000;
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss (NOLOAD) : { *(.bss*) *(COMMON) }

    /DISCARD/ : { *(.comment) *(.riscv.attributes) }
}

ASSERT(SIZEOF(.data) + SIZEOF(.bss) + SIZEOF(.rodata) <= 131072,
       "Static data exceeds 128 KiB")
