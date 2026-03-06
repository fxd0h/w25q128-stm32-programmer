# W25Q128 H7 — Hoja de Ruta del Proyecto

## Visión General

Firmware para la placa **NUCLEO-H743ZI2** que expone una interfaz **USB CDC ACM** (puerto serie virtual) para comunicarse con un host PC, con el objetivo final de programar una memoria flash **W25Q128** vía **QUADSPI**.

## Arquitectura

```
┌──────────────┐     USB CDC      ┌───────────────┐     QUADSPI      ┌──────────┐
│   Host PC    │ ◄──────────────► │  STM32H743ZI  │ ◄──────────────► │ W25Q128  │
│  (terminal)  │   Virtual COM    │  NUCLEO Board  │   Quad SPI Bus   │  Flash   │
└──────────────┘                  └───────────────┘                  └──────────┘
```

## Hardware

| Componente | Detalle |
|---|---|
| **MCU** | STM32H743ZIT (Cortex-M7 @ 480 MHz) |
| **Board** | NUCLEO-H743ZI2 |
| **Flash** | W25Q128 (128 Mbit / 16 MB) |
| **USB** | USB OTG FS (CN13 — conector USB User) |
| **Debug** | ST-Link V3 integrado (CN1 — conector USB ST-Link) |

## Fases del Proyecto

### Fase 1: USB CDC Funcional ← **ACTUAL**
- [x] Proyecto CubeMX generado con USBX CDC ACM
- [ ] Compilación exitosa del firmware
- [ ] Flash y debug via ST-Link
- [ ] USB CDC echo test (enviar datos → recibirlos de vuelta)
- [ ] LED de status (green=running, yellow=USB connected, red=error)

### Fase 2: CLI sobre USB CDC
- [ ] Parser de comandos sobre USB CDC
- [ ] Comando `ping` → responde `pong`
- [ ] Comando `info` → versión firmware, clocks, uptime
- [ ] Comando `led <color> <on|off>` → control de LEDs

### Fase 3: W25Q128 Driver via QUADSPI
- [ ] Lectura de JEDEC ID (sanity check)
- [ ] Read / Write / Erase en modo Single-SPI
- [ ] Upgrade a Quad-SPI
- [ ] Comandos CLI: `flash read`, `flash write`, `flash erase`, `flash id`

### Fase 4: Programador de Flash
- [ ] Protocolo binario para bulk transfer
- [ ] Verificación con checksum (CRC32)
- [ ] Script Python en host para enviar archivos .bin

## Stack Tecnológico

| Capa | Tecnología |
|---|---|
| **RTOS** | Standalone (bare-metal con USBX standalone) |
| **USB** | USBX (Azure RTOS) CDC ACM — modo standalone |
| **Build** | CMake + Ninja + arm-none-eabi-gcc 13.3 |
| **Flash/Debug** | STM32CubeProgrammer CLI / OpenOCD + GDB |
| **Generación** | STM32CubeMX 6.17.0 |

## Convenciones

- Todo el código de usuario va entre marcadores `/* USER CODE BEGIN */` y `/* USER CODE END */`
- Cada milestone se commitea con mensaje descriptivo
- Tests de integración verifican funcionalidad end-to-end
- Los archivos generados por CubeMX NO se modifican fuera de las zonas USER CODE
