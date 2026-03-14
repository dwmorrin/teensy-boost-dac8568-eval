import math
import time
import logging
import sys

# Import the driver and your auto-detect helper
from driver import DAC8568Driver 
from find_teensy_port import find_teensy_port

logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

def main():
    print("Searching for Teensy Eval Board...")
    port = find_teensy_port()
    
    if not port:
        print("ERROR: Could not find a Teensy port. Please check the USB connection.")
        sys.exit(1)
        
    print(f"Connecting to DAC on {port}...")
    
    with DAC8568Driver(port=port, dummy_mode=False) as dac:
        print("Starting 8-Channel Sine Wave LFO...")
        print("Press Ctrl+C to stop.")

        try:
            start_time = time.time()

            while True:
                t = time.time() - start_time

                # 1. Pre-load data into ALL 8 DAC Channels (0 through 7)
                for ch in range(8):
                    # Offset each channel's phase by 1/8th of a circle
                    phase = (t * 2.0) + (ch * math.pi / 4.0) 
                    
                    # Calculate sine wave from 0.0 to 1.0
                    sine_value = (math.sin(phase) + 1.0) / 2.0 
                    
                    # Scale to 16-bit DAC range
                    raw_value = int(sine_value * 65535)

                    dac.set_channel(ch, raw_value, auto_sync=False)

                # 2. Fire the hardware trigger!
                dac.sync()

                # 3. Cap the framerate (~50 Hz)
                time.sleep(0.02)

        except KeyboardInterrupt:
            print("\nStopping LFO...")
            
        finally:
            print("Zeroing all channels...")
            for ch in range(8):
                dac.set_channel(ch, 0, auto_sync=False)
            dac.sync()
            print("Done.")

if __name__ == "__main__":
    main()