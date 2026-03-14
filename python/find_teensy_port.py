import serial.tools.list_ports


def find_teensy_port():
    """Helper to auto-detect a Teensy serial port"""
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        # Heuristic: Teensy often shows up with "Teensy" or specific VID/PID
        if "Teensy" in p.description or "USB Serial" in p.description:
            return p.device

    # Fallback: Just return the first available, or ask user
    if ports:
        return ports[0].device
    return None


if __name__ == "__main__":
    print("DEBUG: Attempting to find Teensy...")
    found_port = find_teensy_port()

    if found_port:
        print(f"SUCCESS: Teensy found on port: {found_port}")
    else:
        print("FAILURE: No Teensy detected (or no serial ports available).")
