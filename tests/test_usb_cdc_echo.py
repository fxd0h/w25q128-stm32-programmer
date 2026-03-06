#!/usr/bin/env python3
"""USB CDC Echo Test for W25Q128 H7 project.

Sends test strings to the USB CDC port and verifies echo response.
Exit code 0 = all tests passed, 1 = failure.
"""
import serial
import time
import sys
import glob

def find_cdc_port():
    """Find the USB CDC port."""
    ports = glob.glob('/dev/cu.usbmodem*')
    if not ports:
        print("ERROR: No USB CDC device found (/dev/cu.usbmodem*)")
        return None
    print(f"Found CDC port: {ports[0]}")
    return ports[0]

def test_echo(port_name):
    """Test echo functionality."""
    results = []
    
    try:
        ser = serial.Serial(port_name, 115200, timeout=3)
        time.sleep(1)  # Wait for device to be ready
        
        # Flush any stale data
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        test_cases = [
            b"Hello W25Q!",
            b"ABCDEFGHIJ",
            b"\x01\x02\x03\x04\x05",
            b"A" * 64,
        ]
        
        for i, test_data in enumerate(test_cases):
            ser.reset_input_buffer()
            time.sleep(0.1)
            
            ser.write(test_data)
            ser.flush()
            time.sleep(0.5)
            
            response = ser.read(len(test_data) + 10)
            
            passed = test_data in response
            label = f"Test {i+1}: {len(test_data)} bytes"
            
            if passed:
                print(f"  PASS  {label}")
            else:
                print(f"  FAIL  {label}")
                print(f"        Sent:     {test_data[:40]!r}...")
                print(f"        Received: {response[:40]!r}...")
            
            results.append(passed)
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"ERROR: Serial port error: {e}")
        return False
    except Exception as e:
        print(f"ERROR: {e}")
        return False
    
    return all(results)

def main():
    print("=" * 50)
    print("W25Q128 H7 — USB CDC Echo Test")
    print("=" * 50)
    
    port = find_cdc_port()
    if not port:
        sys.exit(1)
    
    print("\nRunning echo tests...")
    if test_echo(port):
        print("\n✅ ALL TESTS PASSED")
        sys.exit(0)
    else:
        print("\n❌ SOME TESTS FAILED")
        sys.exit(1)

if __name__ == "__main__":
    main()
