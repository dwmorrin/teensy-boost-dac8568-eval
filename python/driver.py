import serial
import struct
import time
import logging

logger = logging.getLogger(__name__)

class DAC8568Driver:
    def __init__(self, port, dummy_mode=False, vref=2.5):
        self.port = port
        self.dummy_mode = dummy_mode
        self.vref = vref
        self.ser: serial.Serial | None = None

        # Shadow registers for all 8 DAC channels (0-7)
        self._shadow_registers = [0x0000] * 8

        # Protocol Headers
        self.HEADER_DAC_WRITE = 0xB5
        self.HEADER_LDAC_SYNC = 0xC5
        self.HEADER_DONE = 0xAA

        if not self.dummy_mode:
            try:
                self._connect()
            except Exception as e:
                logger.warning(f"Initial connection failed ({e}).")

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def _connect(self):
        if self.ser and self.ser.is_open:
            self.ser.close()

        logger.info(f"Connecting to DAC Eval Board on {self.port}...")
        self.ser = serial.Serial(self.port, 9600, timeout=2.0)
        self.ser.reset_input_buffer()
        time.sleep(1.0)

    def close(self):
        if self.ser:
            self.ser.close()

    def set_channel(self, channel: int, raw_value: int, auto_sync=False):
        """
        Loads a 16-bit raw value into a specific DAC channel's input buffer.
        """
        if not 0 <= channel <= 7:
            raise ValueError("Channel must be between 0 and 7")
        if not 0 <= raw_value <= 65535:
            raise ValueError("Value must be a 16-bit integer (0-65535)")

        self._shadow_registers[channel] = raw_value
        
        if self.dummy_mode:
            logger.info(f"[MOCK] Loaded Ch {channel} with raw value: {raw_value}")
        else:
            # Struct Format: > (Big Endian), B (1 byte int), H (2 byte int)
            # Packet layout: [Header 1B] [Channel 1B] [Value 2B] [Padding 1B]
            payload = struct.pack(">B B H B", self.HEADER_DAC_WRITE, channel, raw_value, 0)
            self._write_and_confirm(payload)

        if auto_sync:
            self.sync()

    def set_voltage(self, channel: int, voltage: float, auto_sync=False):
        """
        Helper method to calculate the 16-bit code for a desired target voltage.
        """
        if not 0 <= voltage <= self.vref:
            logger.warning(f"Voltage {voltage}V is outside the 0-{self.vref}V range. Clamping.")
            voltage = max(0.0, min(voltage, self.vref))

        raw_value = int((voltage / self.vref) * 65535)
        self.set_channel(channel, raw_value, auto_sync)

    def sync(self):
        """
        Fires the hardware ~LDAC pin on the Teensy to update all DAC outputs simultaneously.
        """
        if self.dummy_mode:
            logger.info("[MOCK] Synchronized DAC outputs (LDAC Triggered)")
            return

        # Packet layout: [Header 1B] [4 Bytes Padding]
        payload = struct.pack(">B I", self.HEADER_LDAC_SYNC, 0)
        self._write_and_confirm(payload)

    def _write_and_confirm(self, payload: bytes, retry_count=1):
        try:
            if self.ser is None:
                raise serial.SerialException("Device not connected")

            self.ser.reset_input_buffer()
            self.ser.write(payload)

            # Wait for Done packet
            done_packet = self.ser.read(5)
            if len(done_packet) != 5 or done_packet[0] != self.HEADER_DONE:
                raise serial.SerialTimeoutException("Write failed (No or invalid DONE packet).")

        except (serial.SerialException, OSError) as e:
            logger.warning(f"Comms Error ({e}). Attempting to reconnect...")
            if retry_count > 0:
                try:
                    self._connect()
                    self._write_and_confirm(payload, retry_count=0)
                except Exception as reconnect_error:
                    logger.error(f"Reconnect failed: {reconnect_error}")
                    raise
            else:
                raise