# Integer widths

Three widths matter, and they describe different things:

| Layer | Current implementation |
|---|---|
| EFI host | x86-64 CPU execution and pointers |
| Emulated machine | 8-bit 6502 with a 16-bit address space |
| BASIC arithmetic | Original Microsoft interpreter numeric behavior |

`BITS` controls a separate exact-integer calculator. It does not change BASIC
variables, expand BASIC's address space, or switch the physical x86 CPU into a
different mode.

```text
BITS 8
CALC 255+1
UNSIGNED 0  SIGNED 0  HEX $00
BITS 16
CALC $FFFF
UNSIGNED 65535  SIGNED -1  HEX $FFFF
BITS 64
CALC $FFFFFFFFFFFFFFFF
UNSIGNED 18446744073709551615  SIGNED -1  HEX $FFFFFFFFFFFFFFFF
```

Valid widths are 8, 16, 32, 64 and NATIVE. NATIVE means the host pointer width,
currently 64 for EFI. Enter `BITS` with no argument to show the current setting.

## Arithmetic rules

- Decimal and hexadecimal (`$` or `0x`) literals are accepted.
- Every operation and literal is reduced modulo 2 raised to the selected width.
- The result is shown as unsigned, signed two's-complement, and hexadecimal.
- Division and remainder are unsigned; right shift is logical.
- Operators are `* / % + - << >> & ^ |`, with unary `+ - ~` and parentheses.
- Precedence follows the corresponding C operators. Use parentheses when unsure.
- Division by zero and shift counts at least the selected width are errors.

For example, `CALC -1` in 8-bit mode displays 255, -1, and FF. That is three
interpretations of the same eight bits. It does not turn BASIC's `PRINT -1`
into unsigned arithmetic.

A future BASIC dialect with explicit wide integer types needs its own design
and compatibility tests. It is not already implemented by this selector.
