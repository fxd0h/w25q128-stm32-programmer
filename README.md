# W25Q128 SPI Flash Programmer

STM32H743 Nucleo-144 based SPI flash programmer for W25Q128 (16MB) chips.
Built for BIOS recovery of LattePanda Sigma boards on macOS (where CH341A is unreliable).

## Features

- **USB CDC** command interface — no drivers needed, works on macOS/Linux/Windows
- **MCU-side write+verify** — programs pages and verifies readback internally on STM32, no USB round-trip for verification
- **209 KB/s** write+verify throughput (16MB in ~78s)
- **362 KB/s** read/verify-only throughput
- Auto-retry with protocol resync on CDC errors
- SPI at 30 MHz (prescaler /4)

## Wiring

### W25Q128 SOIC-8 Chip Pinout

```
        W25Q128 (top view)
        ┌──── dot = pin 1
        ▼
      ┌────────┐
  CS# │1 ●    8│ VCC
   DO │2      7│ HOLD#
  WP# │3      6│ CLK
  GND │4      5│ DI
      └────────┘
```

### Wiring to STM32H743 Nucleo-144

```
  W25Q128                          STM32 Nucleo-144
  ┌────────┐
  │1  CS#  │─────────────────────► PA10
  │2  DO   │─────────────────────► PB4  (SPI1 MISO)
  │3  WP#  │─────┐
  │4  GND  │──┐  ├───────────────► PA3  (GPIO HIGH)
  │5  DI   │  │  │
  │6  CLK  │  │  │
  │7 HOLD# │──┼──┘
  │8  VCC  │  │
  └────────┘  │
              │
  Pin 5 DI ───┼──────────────────► PB5  (SPI1 MOSI)
  Pin 6 CLK ──┼──────────────────► PB3  (SPI1 SCK)
  Pin 8 VCC ──┼──────────────────► PA2  (GPIO HIGH, 3.3V power)
  Pin 4 GND ──└──────────────────► GND
```

| W25Q128 Pin | Signal | STM32 Pin | Notes |
|---|---|---|---|
| 1 | CS# | **PA10** | Chip select (active low) |
| 2 | DO (MISO) | **PB4** | SPI1_MISO |
| 3 | WP# | **PA3** | Tied HIGH (write protect disabled) |
| 4 | GND | **GND** | Ground — **don't forget this one!** |
| 5 | DI (MOSI) | **PB5** | SPI1_MOSI |
| 6 | CLK | **PB3** | SPI1_SCK |
| 7 | HOLD# | **PA3** | Tied HIGH with WP# (same wire) |
| 8 | VCC | **PA2** | 3.3V power from GPIO (~15mA) |

### In-Circuit Programming (SOIC-8 Clip)

To program a chip soldered on a board (e.g. LattePanda):

1. **Power off** the target board completely (disconnect power supply + RTC battery)
2. Attach the SOIC-8 clip to the W25Q128 — align pin 1 (red wire) with the dot on the chip
3. The STM32 provides 3.3V through PA2 — no external power needed
4. Run `python3 tools/flash_w25q.py id` to verify connection (expect `EF 40 18`)
5. If you get `FF FF FF`, reposition the clip for better contact

> **⚠️ Important**: The target board's chipset may load the SPI bus even when powered off. If the clip reads intermittently, try applying gentle pressure or slightly repositioning it.

### Requirements

- STM32H743ZI Nucleo-144 board
- USB cable (board to Mac/PC)
- ST-LINK for firmware flashing
- SOIC-8 clip or direct wiring to W25Q128

## Python Tool

```bash
# Setup
cd tools
python3 -m venv .venv && source .venv/bin/activate
pip install pyserial

# Commands
python3 tools/flash_w25q.py ping                        # Test connection
python3 tools/flash_w25q.py id                          # Read JEDEC ID
python3 tools/flash_w25q.py program firmware.bin         # Program + MCU-verify
python3 tools/flash_w25q.py verify firmware.bin          # Verify against file
python3 tools/flash_w25q.py read output.bin -s 0x1000   # Read flash to file
python3 tools/flash_w25q.py erase                       # Erase chip
python3 tools/flash_w25q.py selftest -b 4               # Internal SPI self-test (4 blocks)
```

## Firmware Protocol (USB CDC)

| Command | Code | Format | Response |
|---------|------|--------|----------|
| PING | `0xFF` | `[FF]` | `"OK"` |
| JEDEC_ID | `0x01` | `[01]` | `[ACK][MFR][TYPE][CAP]` |
| READ | `0x02` | `[02][ADDR:3][LEN:2]` | `[ACK][DATA:LEN]` |
| ERASE_4K | `0x03` | `[03][ADDR:3]` | `[ACK]` |
| ERASE_64K | `0x06` | `[06][ADDR:3]` | `[ACK]` |
| PAGE_PROG | `0x04` | `[04][ADDR:3][LEN:1][DATA]` | `[ACK]` |
| PROG_VERIFY | `0x11` | `[11][ADDR:3][LEN:2][DATA]` | `[ACK]` or `[ERR][OFF:2]` |
| CHIP_ERASE | `0x05` | `[05]` | `[ACK]` |
| SELF_TEST | `0x10` | `[10][BLOCKS:1]` | `[ACK][PASS:4]` or `[ERR]` |

## Performance

| Operation | Speed | 16MB Time |
|-----------|-------|-----------|
| Erase | 384 KB/s | ~42s |
| Program + MCU-verify | 209 KB/s | ~78s |
| Read/Verify only | 362 KB/s | ~45s |
| **Total flash cycle** | | **~120s** |

## Building

```bash
cmake --build build/Debug
STM32_Programmer_CLI -c port=SWD -w build/Debug/w25q128h7.elf -v -rst
STM32_Programmer_CLI -c port=SWD -hardRst
```

## License

MIT
