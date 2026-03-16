#!/usr/bin/env python3
"""
W25Q128 SPI Flash Programmer — Host-side Tool
Communicates with STM32H743 via USB CDC to program W25Q128.

Usage:
  flash_w25q.py ping                       Test connection
  flash_w25q.py id                         Read JEDEC ID
  flash_w25q.py read <file> [-a ADDR] [-s SIZE]
  flash_w25q.py program <file> [-a ADDR] [--no-verify]
  flash_w25q.py verify <file> [-a ADDR]
  flash_w25q.py erase [-a ADDR] [-s SIZE]  Erase (4K/64K/chip auto)
  flash_w25q.py dump [-a ADDR] [-s SIZE]   Hex dump to terminal
  flash_w25q.py test [-s SIZE]             Write pattern, verify
"""
import sys
import os
import time
import serial
import serial.tools.list_ports
import argparse

# ── Protocol ──────────────────────────────────────────────
CMD_JEDEC_ID   = 0x01
CMD_READ       = 0x02  # ADDR[3] LEN_HI LEN_LO → ACK data[LEN]
CMD_ERASE_4K   = 0x03  # ADDR[3] → ACK
CMD_PAGE_PROG  = 0x04  # ADDR[3] LEN DATA[LEN] → ACK
CMD_CHIP_ERASE = 0x05  # → ACK
CMD_ERASE_64K  = 0x06  # ADDR[3] → ACK
CMD_SELF_TEST  = 0x10  # → ACK + result (internal SPI test)
CMD_PROG_VERIFY = 0x11  # → Write + verify on MCU (no USB round-trip)
CMD_PING       = 0xFF  # → "OK"

ACK_OK  = 0x06
ACK_ERR = 0x15

PAGE_SIZE   = 256
SECTOR_SIZE = 4096
BLOCK_SIZE  = 65536
CHIP_SIZE   = 16 * 1024 * 1024  # 16 MB
READ_CHUNK  = 256


# ── Helpers ───────────────────────────────────────────────
def parse_int(s):
    """Parse int with 0x support."""
    s = s.strip()
    if s.lower().startswith('0x'):
        return int(s, 16)
    return int(s)


def human_size(n):
    if n >= 1024 * 1024:
        return f'{n / 1024 / 1024:.1f} MB'
    elif n >= 1024:
        return f'{n / 1024:.1f} KB'
    return f'{n} B'


def progress(current, total, prefix='', t0=None):
    pct = current / total if total > 0 else 1
    bar_w = 40
    filled = int(bar_w * pct)
    bar = '█' * filled + '░' * (bar_w - filled)
    speed = ''
    if t0:
        elapsed = time.time() - t0
        if elapsed > 0.5 and current > 0:
            bps = current / elapsed
            speed = f' {bps/1024:.0f} KB/s'
            remaining = (total - current) / bps if bps > 0 else 0
            if remaining > 60:
                speed += f' ~{remaining/60:.0f}m left'
            elif remaining > 1:
                speed += f' ~{remaining:.0f}s left'
    sys.stdout.write(f'\r{prefix} |{bar}| {pct*100:5.1f}%{speed}  ')
    sys.stdout.flush()
    if current >= total:
        print()


def find_cdc_port():
    """Auto-detect CDC port."""
    import glob
    ports = sorted(glob.glob('/dev/cu.usbmodem*'))
    cdc = [p for p in ports if '000000000001' in p]
    return cdc[0] if cdc else (ports[-1] if ports else None)


# ── Flasher Class ─────────────────────────────────────────
class W25QFlasher:
    def __init__(self, port, baudrate=115200, timeout=3):
        self.ser = serial.Serial(port, baudrate, timeout=timeout)
        time.sleep(0.1)
        self.ser.reset_input_buffer()

    def close(self):
        self.ser.close()

    def _cmd(self, data, expect, timeout=None):
        old = self.ser.timeout
        if timeout:
            self.ser.timeout = timeout
        self.ser.reset_input_buffer()
        self.ser.write(data)
        self.ser.flush()
        resp = self.ser.read(expect)
        if timeout:
            self.ser.timeout = old
        return resp

    def ping(self):
        return self._cmd(bytes([CMD_PING]), 2) == b'OK'

    def jedec_id(self):
        r = self._cmd(bytes([CMD_JEDEC_ID]), 4)
        if len(r) >= 4 and r[0] == ACK_OK:
            return (r[1], r[2], r[3])
        return None

    def read(self, addr, length):
        cmd = bytes([CMD_READ,
                     (addr >> 16) & 0xFF,
                     (addr >> 8) & 0xFF,
                     addr & 0xFF,
                     (length >> 8) & 0xFF,
                     length & 0xFF])
        r = self._cmd(cmd, 1 + length)
        if len(r) >= 1 and r[0] == ACK_OK:
            return r[1:]
        return None

    def erase_4k(self, addr):
        cmd = bytes([CMD_ERASE_4K,
                     (addr >> 16) & 0xFF,
                     (addr >> 8) & 0xFF,
                     addr & 0xFF])
        r = self._cmd(cmd, 1, timeout=2)
        return len(r) >= 1 and r[0] == ACK_OK

    def erase_64k(self, addr):
        cmd = bytes([CMD_ERASE_64K,
                     (addr >> 16) & 0xFF,
                     (addr >> 8) & 0xFF,
                     addr & 0xFF])
        r = self._cmd(cmd, 1, timeout=5)
        return len(r) >= 1 and r[0] == ACK_OK

    def erase_chip(self):
        r = self._cmd(bytes([CMD_CHIP_ERASE]), 1, timeout=120)
        return len(r) >= 1 and r[0] == ACK_OK

    def page_program(self, addr, data):
        """Program up to 256 bytes. Uses careful timing for USB reliability."""
        assert len(data) <= PAGE_SIZE
        plen = len(data) if len(data) < 256 else 0
        header = bytes([CMD_PAGE_PROG,
                        (addr >> 16) & 0xFF,
                        (addr >> 8) & 0xFF,
                        addr & 0xFF,
                        plen])
        # Send header + data as one write for USB coherence
        self.ser.write(header + bytes(data))
        self.ser.flush()
        # Wait for ACK — firmware needs time to accumulate USB packets
        # and execute SPI page program (~3ms at 750KHz + program time)
        r = self.ser.read(1)
        return len(r) >= 1 and r[0] == ACK_OK

    def prog_verify(self, addr, data):
        """Write + verify on MCU. Sends up to 4KB, MCU programs pages
           and reads back internally. No USB round-trip for verify."""
        dlen = len(data)
        assert dlen <= 4096
        header = bytes([CMD_PROG_VERIFY,
                        (addr >> 16) & 0xFF,
                        (addr >> 8) & 0xFF,
                        addr & 0xFF,
                        (dlen >> 8) & 0xFF,
                        dlen & 0xFF])
        self.ser.write(header + bytes(data))
        self.ser.flush()
        # Timeout: 16 page programs × 3ms + read-back time
        r = self.ser.read(1)
        if len(r) >= 1 and r[0] == ACK_OK:
            return True
        # Error — read fail offset if available
        extra = self.ser.read(2)
        if len(extra) >= 2:
            fail_off = (extra[0] << 8) | extra[1]
            print(f'\n  MCU verify fail at offset +0x{fail_off:04X} (addr 0x{addr+fail_off:06X})')
        return False

    def _resync(self):
        """Re-synchronize CDC protocol after error."""
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()
        time.sleep(0.05)
        self.ser.reset_input_buffer()
        # Send ping to re-establish sync
        for _ in range(3):
            self.ser.write(bytes([CMD_PING]))
            self.ser.flush()
            time.sleep(0.05)
            r = self.ser.read(2)
            if r == b'OK':
                return True
            self.ser.reset_input_buffer()
            time.sleep(0.1)
        return False

    def page_program_verified(self, addr, data, max_retries=3):
        """Program page + verify readback. Retry on failure."""
        for attempt in range(max_retries):
            ok = self.page_program(addr, data)
            if not ok:
                print(f'\n  WARN: page_program NAK at 0x{addr:06X}, retrying...')
                self._resync()
                continue
            # Read back and verify
            rd = self.read(addr, len(data))
            if rd is not None and rd[:len(data)] == data:
                return True
            # Mismatch — resync and retry
            if attempt < max_retries - 1:
                print(f'\n  WARN: verify fail at 0x{addr:06X} attempt {attempt+1}, resyncing...')
                self._resync()
                time.sleep(0.05)
        return False

    def self_test(self, num_blocks=1):
        """Run internal SPI self-test on MCU (no CDC data involved)."""
        # Timeout: erase ~1s/block + program ~2s/block + verify ~1s/block
        timeout = max(10, num_blocks * 5)
        r = self._cmd(bytes([CMD_SELF_TEST, num_blocks]), 5, timeout=timeout)
        return r


# ── Commands ──────────────────────────────────────────────
def do_ping(f, args):
    print('Ping OK' if f.ping() else 'Ping FAILED')


def do_id(f, args):
    jid = f.jedec_id()
    if not jid:
        print('ERROR: JEDEC ID read failed')
        return False
    mfr, typ, cap = jid
    info = ''
    if mfr == 0xEF: info += ' Winbond'
    if cap == 0x18: info += ' W25Q128 (16MB)'
    elif cap == 0x17: info += ' W25Q64 (8MB)'
    elif cap == 0x16: info += ' W25Q32 (4MB)'
    print(f'JEDEC ID: {mfr:02X} {typ:02X} {cap:02X}{info}')
    return True


def do_read(f, args):
    addr = parse_int(args.addr) if args.addr else 0
    size = parse_int(args.size) if args.size else CHIP_SIZE
    fname = args.file
    print(f'Reading {human_size(size)} from 0x{addr:06X} to {fname}')
    t0 = time.time()
    with open(fname, 'wb') as fp:
        offset = 0
        while offset < size:
            chunk = min(READ_CHUNK, size - offset)
            data = f.read(addr + offset, chunk)
            if data is None or len(data) < chunk:
                print(f'\nERROR: read failed at 0x{addr+offset:06X}')
                return False
            fp.write(data)
            offset += chunk
            progress(offset, size, 'Reading', t0)
    el = time.time() - t0
    print(f'Done: {human_size(size)} in {el:.1f}s ({size/el/1024:.1f} KB/s)')
    return True


def do_dump(f, args):
    addr = parse_int(args.addr) if args.addr else 0
    size = parse_int(args.size) if args.size else 256
    if size > 4096:
        print('Dump limited to 4KB max, use read for larger')
        size = 4096
    offset = 0
    while offset < size:
        chunk = min(READ_CHUNK, size - offset)
        data = f.read(addr + offset, chunk)
        if data is None:
            print(f'\nERROR at 0x{addr+offset:06X}')
            return False
        for i in range(0, len(data), 16):
            row = data[i:i+16]
            hex_part = ' '.join(f'{b:02X}' for b in row)
            ascii_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in row)
            print(f'  {addr+offset+i:06X}: {hex_part:<48s} |{ascii_part}|')
        offset += chunk
    return True


def do_erase(f, args):
    addr = parse_int(args.addr) if args.addr else 0
    size = parse_int(args.size) if args.size else CHIP_SIZE

    if size >= CHIP_SIZE and addr == 0:
        print('Erasing entire chip (~40s)...')
        t0 = time.time()
        ok = f.erase_chip()
        print(f'{"OK" if ok else "FAILED"} ({time.time()-t0:.1f}s)')
        return ok

    # Use 64K blocks where aligned, 4K sectors for remainder
    t0 = time.time()
    erased = 0
    cur = addr
    end = addr + size

    print(f'Erasing {human_size(size)} from 0x{addr:06X}...')
    while cur < end:
        remaining = end - cur
        if cur % BLOCK_SIZE == 0 and remaining >= BLOCK_SIZE:
            ok = f.erase_64k(cur)
            step = BLOCK_SIZE
            label = '64K'
        elif cur % SECTOR_SIZE == 0 and remaining >= SECTOR_SIZE:
            ok = f.erase_4k(cur)
            step = SECTOR_SIZE
            label = '4K'
        else:
            # Align down to sector
            aligned = cur & ~(SECTOR_SIZE - 1)
            ok = f.erase_4k(aligned)
            step = SECTOR_SIZE - (cur - aligned)
            if cur + step > end:
                step = end - cur
            label = '4K'

        if not ok:
            print(f'\nERASE FAILED at 0x{cur:06X}')
            return False
        cur += step
        erased += step
        progress(min(erased, size), size, 'Erasing', t0)

    print(f'Done: {time.time()-t0:.1f}s')
    return True


def do_program(f, args):
    fname = args.file
    addr = parse_int(args.addr) if args.addr else 0
    fsize = os.path.getsize(fname)

    with open(fname, 'rb') as fp:
        data = fp.read()

    print(f'Programming {fname} ({human_size(fsize)}) at 0x{addr:06X}')

    # 1. Erase
    erase_size = fsize
    # Align to 64K blocks for speed
    print(f'Step 1/3: Erasing {human_size(erase_size)}...')
    t0 = time.time()
    cur = addr
    end = addr + erase_size
    erased = 0
    while cur < end:
        remaining = end - cur
        if cur % BLOCK_SIZE == 0 and remaining >= BLOCK_SIZE:
            ok = f.erase_64k(cur)
            step = BLOCK_SIZE
        else:
            ok = f.erase_4k(cur)
            step = SECTOR_SIZE
        if not ok:
            print(f'\nERASE FAILED at 0x{cur:06X}')
            return False
        cur += step
        erased += step
        progress(min(erased, erase_size), erase_size, 'Erasing', t0)
    print(f'  Erase: {time.time()-t0:.1f}s')

    # 2. Program (write+verify on MCU, 4KB chunks)
    VERIFY_CHUNK = SECTOR_SIZE  # 4KB
    print(f'Step 2/2: Programming + MCU-verify {human_size(fsize)} (4KB chunks)...')
    t0 = time.time()
    offset = 0
    while offset < fsize:
        chunk_size = min(VERIFY_CHUNK, fsize - offset)
        chunk_data = data[offset:offset+chunk_size]
        success = False
        for attempt in range(3):
            ok = f.prog_verify(addr + offset, chunk_data)
            if ok:
                success = True
                break
            if attempt < 2:
                print(f'\n  WARN: prog_verify fail at 0x{addr+offset:06X}, attempt {attempt+1}')
                f._resync()
                time.sleep(0.05)
        if not success:
            print(f'\nPROGRAM FAILED at 0x{addr+offset:06X} (after retries)')
            return False
        offset += chunk_size
        progress(offset, fsize, 'Write+Verify', t0)
    print(f'  Program+Verify: {time.time()-t0:.1f}s')

    print('*** PROGRAMMING COMPLETE ***')
    return True


def _verify(f, data, addr, size):
    t0 = time.time()
    offset = 0
    while offset < size:
        chunk = min(READ_CHUNK, size - offset)
        rd = f.read(addr + offset, chunk)
        if rd is None:
            print(f'\nREAD FAILED at 0x{addr+offset:06X}')
            return False
        expected = data[offset:offset+chunk]
        if rd[:len(expected)] != expected:
            for j in range(len(expected)):
                if j < len(rd) and rd[j] != expected[j]:
                    print(f'\n  MISMATCH at 0x{addr+offset+j:06X}: '
                          f'expected 0x{expected[j]:02X} got 0x{rd[j]:02X}')
                    return False
        offset += chunk
        progress(offset, size, 'Verifying', t0)
    print(f'  Verify: {time.time()-t0:.1f}s — OK')
    return True


def do_verify(f, args):
    fname = args.file
    addr = parse_int(args.addr) if args.addr else 0
    fsize = os.path.getsize(fname)
    with open(fname, 'rb') as fp:
        data = fp.read()
    print(f'Verifying {fname} ({human_size(fsize)}) at 0x{addr:06X}')
    return _verify(f, data, addr, fsize)


def do_test(f, args):
    size = parse_int(args.size) if args.size else 128 * 1024
    addr = parse_int(args.addr) if args.addr else 0
    print(f'=== TEST: {human_size(size)} at 0x{addr:06X} ===')

    # Generate pattern
    pattern = bytes([(i & 0xFF) for i in range(size)])

    # Erase
    print(f'1. Erasing...')
    t0 = time.time()
    cur = addr
    end = addr + size
    erased = 0
    while cur < end:
        remaining = end - cur
        if cur % BLOCK_SIZE == 0 and remaining >= BLOCK_SIZE:
            ok = f.erase_64k(cur)
            step = BLOCK_SIZE
        else:
            ok = f.erase_4k(cur)
            step = SECTOR_SIZE
        if not ok:
            print(f'   ERASE FAILED at 0x{cur:06X}')
            return False
        cur += step
        erased += step
        progress(min(erased, size), size, '  Erase', t0)
    print(f'   Erase: {time.time()-t0:.1f}s')

    # Verify erased (FF)
    print('2. Checking erased (all 0xFF)...')
    t0 = time.time()
    offset = 0
    while offset < size:
        chunk = min(READ_CHUNK, size - offset)
        rd = f.read(addr + offset, chunk)
        if rd is None:
            print(f'   READ FAILED at 0x{addr+offset:06X}')
            return False
        for j, b in enumerate(rd[:chunk]):
            if b != 0xFF:
                print(f'   NOT ERASED at 0x{addr+offset+j:06X}: 0x{b:02X}')
                return False
        offset += chunk
        progress(offset, size, '  Verify', t0)
    print(f'   Erase verify OK')

    # Program
    print('3. Programming pattern...')
    t0 = time.time()
    offset = 0
    while offset < size:
        plen = min(PAGE_SIZE, size - offset)
        ok = f.page_program(addr + offset, pattern[offset:offset+plen])
        if not ok:
            print(f'   PROGRAM FAILED at 0x{addr+offset:06X}')
            return False
        offset += plen
        progress(offset, size, '  Write', t0)
    print(f'   Program: {time.time()-t0:.1f}s')

    # Verify
    print('4. Verifying pattern...')
    ok = _verify(f, pattern, addr, size)
    if ok:
        print(f'=== TEST PASSED: {human_size(size)} ===')
    else:
        print(f'=== TEST FAILED ===')
    return ok


def do_selftest(f, args):
    num_blocks = int(args.blocks) if args.blocks else 2
    size_kb = num_blocks * 64
    print(f'Running internal SPI self-test: {num_blocks} blocks ({size_kb} KB)...')
    print(f'(Erase + Write + Verify — no CDC data transfer)')
    r = f.self_test(num_blocks)
    if len(r) >= 5 and r[0] == ACK_OK:
        passed = (r[1] << 24) | (r[2] << 16) | (r[3] << 8) | r[4]
        total = num_blocks * 64 * 1024
        print(f'SELF-TEST PASSED: {passed}/{total} bytes verified')
        return True
    elif len(r) >= 2 and r[0] == ACK_ERR:
        codes = {1: 'Erase failed', 2: 'Read failed after erase',
                 3: 'Erase verify failed (not all 0xFF)',
                 4: 'Page program failed', 5: 'Read failed after program',
                 6: 'Data mismatch'}
        code = r[1]
        msg = codes.get(code, f'Unknown error 0x{code:02X}')
        if code == 6 and len(r) >= 5:
            fail_off = (r[2] << 16) | (r[3] << 8) | r[4]
            msg += f' at offset 0x{fail_off:06X}'
        print(f'SELF-TEST FAILED: {msg}')
        return False
    else:
        print(f'SELF-TEST: no response (got {r.hex() if r else "nothing"})')
        return False


# ── Main ──────────────────────────────────────────────────
def main():
    p = argparse.ArgumentParser(
        description='W25Q128 SPI Flash Programmer',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''Examples:
  %(prog)s ping
  %(prog)s id
  %(prog)s dump -a 0x000000 -s 256
  %(prog)s test -s 0x20000              # test 128KB
  %(prog)s erase -a 0 -s 0x10000       # erase 64KB
  %(prog)s read firmware.bin -s 0x10000 # read 64KB
  %(prog)s program bios.bin             # erase+write+verify full file
  %(prog)s verify bios.bin              # verify against file
''')
    p.add_argument('command',
                   choices=['ping', 'id', 'read', 'program', 'verify',
                            'erase', 'dump', 'test', 'selftest'],
                   help='Command')
    p.add_argument('file', nargs='?', help='File for read/program/verify')
    p.add_argument('-p', '--port', help='Serial port (auto-detect)')
    p.add_argument('-a', '--addr', default='0', help='Start address (hex ok)')
    p.add_argument('-s', '--size', default=None, help='Size in bytes (hex ok)')
    p.add_argument('-b', '--blocks', default=None,
                   help='Number of 64KB blocks for selftest (default 2)')
    p.add_argument('--no-verify', action='store_true',
                   help='Skip verify after program')
    args = p.parse_args()

    # Validate
    if args.command in ('read', 'program', 'verify') and not args.file:
        p.error(f'{args.command} requires a file argument')

    port = args.port or find_cdc_port()
    if not port:
        print('ERROR: No CDC port found')
        sys.exit(1)
    print(f'Port: {port}')

    f = W25QFlasher(port)
    if not f.ping():
        print('ERROR: device not responding')
        f.close()
        sys.exit(1)
    print('Connected (ping OK)')

    cmds = {
        'ping':    do_ping,
        'id':      do_id,
        'read':    do_read,
        'dump':    do_dump,
        'erase':   do_erase,
        'program': do_program,
        'verify':  do_verify,
        'test':     do_test,
        'selftest': do_selftest,
    }

    ok = cmds[args.command](f, args)
    f.close()
    sys.exit(0 if ok is not False else 1)


if __name__ == '__main__':
    main()
