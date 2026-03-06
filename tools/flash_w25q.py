#!/usr/bin/env python3
"""
W25Q128 SPI Flash Programmer — Host-side tool
Communicates with STM32H743 via USB CDC to program W25Q128.
"""
import sys
import os
import time
import struct
import serial
import argparse

# Protocol commands
CMD_JEDEC_ID    = 0x01
CMD_READ        = 0x02
CMD_ERASE_4K    = 0x03
CMD_PAGE_PROG   = 0x04
CMD_CHIP_ERASE  = 0x05
CMD_ERASE_64K   = 0x06
CMD_PING        = 0xFF

ACK_OK  = 0x06
ACK_ERR = 0x15

PAGE_SIZE   = 256
SECTOR_SIZE = 4096
BLOCK_SIZE  = 65536
CHIP_SIZE   = 16 * 1024 * 1024  # 16 MB
READ_CHUNK  = 256  # bytes per read command (must fit in 512-1=511 byte response)


def find_cdc_port():
    """Auto-detect the CDC port (not the ST-Link VCP)."""
    import glob
    # ST-Link VCP is usually /dev/cu.usbmodem<lower_number>
    # CDC ACM is usually /dev/cu.usbmodem<higher_number>
    ports = sorted(glob.glob('/dev/cu.usbmodem*'))
    # Filter out ST-Link VCP (usually has 'STLink' or lower serial)
    cdc_ports = [p for p in ports if '000000000001' in p]
    if cdc_ports:
        return cdc_ports[0]
    # Fallback: last port
    if ports:
        return ports[-1]
    return None


class W25QFlasher:
    def __init__(self, port, baudrate=115200, timeout=5):
        self.ser = serial.Serial(port, baudrate, timeout=timeout)
        time.sleep(0.1)
        self.ser.reset_input_buffer()

    def close(self):
        self.ser.close()

    def _send_recv(self, cmd_bytes, expect_len=None, timeout=None):
        """Send command, receive response."""
        if timeout:
            old_timeout = self.ser.timeout
            self.ser.timeout = timeout
        self.ser.reset_input_buffer()
        self.ser.write(cmd_bytes)
        self.ser.flush()
        if expect_len:
            resp = self.ser.read(expect_len)
        else:
            time.sleep(0.05)
            resp = self.ser.read(self.ser.in_waiting or 1)
        if timeout:
            self.ser.timeout = old_timeout
        return resp

    def ping(self):
        resp = self._send_recv(bytes([CMD_PING]), expect_len=2)
        return resp == b'OK'

    def read_jedec_id(self):
        resp = self._send_recv(bytes([CMD_JEDEC_ID]), expect_len=4)
        if len(resp) >= 4 and resp[0] == ACK_OK:
            return resp[1], resp[2], resp[3]
        return None

    def read_flash(self, addr, length):
        """Read up to 511 bytes from flash."""
        cmd = struct.pack('>BBHH',
                          CMD_READ,
                          (addr >> 16) & 0xFF,
                          addr & 0xFFFF,
                          length)
        # Fix: CMD_READ uses 3 addr bytes + 2 len bytes
        cmd = bytes([CMD_READ,
                     (addr >> 16) & 0xFF,
                     (addr >> 8) & 0xFF,
                     addr & 0xFF,
                     (length >> 8) & 0xFF,
                     length & 0xFF])
        resp = self._send_recv(cmd, expect_len=1 + length)
        if len(resp) >= 1 and resp[0] == ACK_OK:
            return resp[1:]
        return None

    def erase_sector(self, addr):
        """Erase 4KB sector."""
        cmd = bytes([CMD_ERASE_4K,
                     (addr >> 16) & 0xFF,
                     (addr >> 8) & 0xFF,
                     addr & 0xFF])
        resp = self._send_recv(cmd, expect_len=1, timeout=2)
        return len(resp) >= 1 and resp[0] == ACK_OK

    def erase_block64(self, addr):
        """Erase 64KB block."""
        cmd = bytes([CMD_ERASE_64K,
                     (addr >> 16) & 0xFF,
                     (addr >> 8) & 0xFF,
                     addr & 0xFF])
        resp = self._send_recv(cmd, expect_len=1, timeout=5)
        return len(resp) >= 1 and resp[0] == ACK_OK

    def erase_chip(self):
        """Full chip erase (~40s)."""
        resp = self._send_recv(bytes([CMD_CHIP_ERASE]),
                               expect_len=1, timeout=120)
        return len(resp) >= 1 and resp[0] == ACK_OK

    def page_program(self, addr, data):
        """Program up to 256 bytes."""
        assert len(data) <= PAGE_SIZE
        cmd = bytes([CMD_PAGE_PROG,
                     (addr >> 16) & 0xFF,
                     (addr >> 8) & 0xFF,
                     addr & 0xFF,
                     len(data)]) + data
        resp = self._send_recv(cmd, expect_len=1, timeout=2)
        return len(resp) >= 1 and resp[0] == ACK_OK


def progress_bar(current, total, prefix='', width=40):
    pct = current / total if total > 0 else 1
    filled = int(width * pct)
    bar = '█' * filled + '░' * (width - filled)
    speed = ''
    sys.stdout.write(f'\r{prefix} |{bar}| {pct*100:5.1f}% {speed}')
    sys.stdout.flush()


def cmd_id(flasher):
    result = flasher.read_jedec_id()
    if result:
        mfr, typ, cap = result
        print(f'JEDEC ID: {mfr:02X} {typ:02X} {cap:02X}', end='')
        if mfr == 0xEF:
            print(' (Winbond)', end='')
        if cap == 0x18:
            print(' W25Q128 (16MB)', end='')
        elif cap == 0x17:
            print(' W25Q64 (8MB)', end='')
        print()
        return True
    else:
        print('ERROR: Failed to read JEDEC ID')
        return False


def cmd_read(flasher, output_file, size=CHIP_SIZE):
    print(f'Reading {size} bytes to {output_file}...')
    t0 = time.time()
    with open(output_file, 'wb') as f:
        addr = 0
        while addr < size:
            chunk = min(READ_CHUNK, size - addr)
            data = flasher.read_flash(addr, chunk)
            if data is None or len(data) != chunk:
                print(f'\nERROR at addr 0x{addr:06X}: read failed')
                return False
            f.write(data)
            addr += chunk
            progress_bar(addr, size, 'Reading')
    elapsed = time.time() - t0
    print(f'\nDone: {size} bytes in {elapsed:.1f}s ({size/elapsed/1024:.1f} KB/s)')
    return True


def cmd_program(flasher, input_file, verify=True):
    filesize = os.path.getsize(input_file)
    print(f'Programming {input_file} ({filesize} bytes = {filesize/1024/1024:.1f} MB)')

    with open(input_file, 'rb') as f:
        data = f.read()

    # Step 1: Erase (using 64KB blocks)
    num_blocks = (filesize + BLOCK_SIZE - 1) // BLOCK_SIZE
    print(f'Erasing {num_blocks} blocks (64KB each)...')
    t0 = time.time()
    for i in range(num_blocks):
        addr = i * BLOCK_SIZE
        if not flasher.erase_block64(addr):
            print(f'\nERROR: Erase failed at 0x{addr:06X}')
            return False
        progress_bar(i + 1, num_blocks, 'Erasing')
    elapsed = time.time() - t0
    print(f'\nErase done: {elapsed:.1f}s')

    # Step 2: Program (256-byte pages)
    num_pages = (filesize + PAGE_SIZE - 1) // PAGE_SIZE
    print(f'Programming {num_pages} pages...')
    t0 = time.time()
    for i in range(num_pages):
        addr = i * PAGE_SIZE
        page_data = data[addr:addr + PAGE_SIZE]
        if not flasher.page_program(addr, page_data):
            print(f'\nERROR: Program failed at 0x{addr:06X}')
            return False
        progress_bar(i + 1, num_pages, 'Programming')
    elapsed = time.time() - t0
    print(f'\nProgram done: {elapsed:.1f}s')

    # Step 3: Verify
    if verify:
        print('Verifying...')
        t0 = time.time()
        addr = 0
        while addr < filesize:
            chunk = min(READ_CHUNK, filesize - addr)
            read_data = flasher.read_flash(addr, chunk)
            if read_data is None:
                print(f'\nERROR: Read failed during verify at 0x{addr:06X}')
                return False
            expected = data[addr:addr + chunk]
            if read_data[:len(expected)] != expected:
                # Find first mismatch
                for j in range(len(expected)):
                    if j < len(read_data) and read_data[j] != expected[j]:
                        print(f'\nMISMATCH at 0x{addr+j:06X}: '
                              f'expected 0x{expected[j]:02X}, '
                              f'got 0x{read_data[j]:02X}')
                        return False
            addr += chunk
            progress_bar(addr, filesize, 'Verifying')
        elapsed = time.time() - t0
        print(f'\nVerify OK: {elapsed:.1f}s')

    print('*** PROGRAMMING COMPLETE ***')
    return True


def cmd_verify(flasher, input_file):
    filesize = os.path.getsize(input_file)
    print(f'Verifying against {input_file} ({filesize} bytes)...')
    with open(input_file, 'rb') as f:
        data = f.read()
    t0 = time.time()
    addr = 0
    while addr < filesize:
        chunk = min(READ_CHUNK, filesize - addr)
        read_data = flasher.read_flash(addr, chunk)
        if read_data is None:
            print(f'\nERROR: Read failed at 0x{addr:06X}')
            return False
        expected = data[addr:addr + chunk]
        if read_data[:len(expected)] != expected:
            for j in range(len(expected)):
                if j < len(read_data) and read_data[j] != expected[j]:
                    print(f'\nMISMATCH at 0x{addr+j:06X}: '
                          f'expected 0x{expected[j]:02X}, got 0x{read_data[j]:02X}')
                    return False
        addr += chunk
        progress_bar(addr, filesize, 'Verifying')
    elapsed = time.time() - t0
    print(f'\nVerify OK: {filesize} bytes match in {elapsed:.1f}s')
    return True


def main():
    parser = argparse.ArgumentParser(description='W25Q128 SPI Flash Programmer')
    parser.add_argument('command', choices=['id', 'read', 'program', 'verify', 'ping', 'erase'],
                        help='Command to execute')
    parser.add_argument('file', nargs='?', help='Input/output file')
    parser.add_argument('-p', '--port', help='Serial port (auto-detect if not specified)')
    parser.add_argument('-s', '--size', type=int, default=CHIP_SIZE,
                        help='Read size in bytes (default: full chip)')
    parser.add_argument('--no-verify', action='store_true',
                        help='Skip verification after programming')
    args = parser.parse_args()

    port = args.port or find_cdc_port()
    if not port:
        print('ERROR: No CDC port found. Is the STM32 connected?')
        sys.exit(1)
    print(f'Using port: {port}')

    flasher = W25QFlasher(port)

    # Always ping first
    if not flasher.ping():
        print('ERROR: Device not responding to ping')
        flasher.close()
        sys.exit(1)
    print('Device connected (ping OK)')

    ok = False
    if args.command == 'ping':
        ok = True
    elif args.command == 'id':
        ok = cmd_id(flasher)
    elif args.command == 'read':
        if not args.file:
            print('ERROR: Output file required for read command')
            sys.exit(1)
        ok = cmd_read(flasher, args.file, args.size)
    elif args.command == 'program':
        if not args.file:
            print('ERROR: Input file required for program command')
            sys.exit(1)
        ok = cmd_program(flasher, args.file, verify=not args.no_verify)
    elif args.command == 'verify':
        if not args.file:
            print('ERROR: Input file required for verify command')
            sys.exit(1)
        ok = cmd_verify(flasher, args.file)
    elif args.command == 'erase':
        print('Erasing entire chip...')
        ok = flasher.erase_chip()
        if ok:
            print('Chip erase complete')
        else:
            print('ERROR: Chip erase failed')

    flasher.close()
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
