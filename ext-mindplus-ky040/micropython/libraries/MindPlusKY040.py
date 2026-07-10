# KY-040 rotary encoder for Mind+ MicroPython (Maixduino / MaixPy)


def _ticks_ms():
    try:
        import time
        return time.ticks_ms()
    except Exception:
        import utime
        return utime.ticks_ms()


def _ticks_diff(a, b):
    try:
        import time
        return time.ticks_diff(a, b)
    except Exception:
        import utime
        return utime.ticks_diff(a, b)


class _PinAdapter:
    def __init__(self, pin_num):
        self._pin_num = int(pin_num)
        self._mode = 'machine'
        self._pin = None
        self._gpio = None
        self._setup()

    def _setup(self):
        # Prefer machine.Pin (MaixPy3 / common MicroPython)
        try:
            from machine import Pin
            try:
                self._pin = Pin(self._pin_num, Pin.IN, Pin.PULL_UP)
            except TypeError:
                self._pin = Pin(self._pin_num, mode=Pin.IN, pull=Pin.PULL_UP)
            self._mode = 'machine'
            return
        except Exception:
            pass

        # Maixduino / MaixPy classic GPIO + fpioa
        try:
            from fpioa_manager import fm
            from Maix import GPIO
            hs = self._pin_num % 32
            fm.register(self._pin_num, getattr(fm.fpioa, 'GPIOHS%d' % hs), force=True)
            try:
                self._gpio = GPIO(getattr(GPIO, 'GPIOHS%d' % hs), GPIO.IN, GPIO.PULL_UP)
            except Exception:
                self._gpio = GPIO(getattr(GPIO, 'GPIOHS%d' % hs), GPIO.IN)
            self._mode = 'maix'
            return
        except Exception:
            pass

        raise RuntimeError('MindPlusKY040: unsupported pin API on this board')

    def value(self):
        if self._mode == 'machine':
            return self._pin.value()
        return self._gpio.value()


class MindPlusKY040:
    STATE_TABLE = (
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0,
    )
    STEPS_PER_DETENT = 4
    BUTTON_DEBOUNCE_MS = 50
    TX_HOLD_MS = 20
    POLL_SAMPLES = 64

    def __init__(self, clk_pin, dt_pin, sw_pin):
        self._clk_pin = int(clk_pin)
        self._dt_pin = int(dt_pin)
        self._sw_pin = int(sw_pin)
        self._initialized = False
        self._tx_open = False
        self._tx_start_ms = 0

        self._count = 0
        self._edge_delta = 0
        self._raw_accum = 0
        self._edge_change = 0
        self._edge_moved = False
        self._edge_cw = False
        self._edge_ccw = False
        self._last_encoded = 0

        self._hold_delta = 0
        self._hold_change = 0
        self._hold_moved = False
        self._hold_cw = False
        self._hold_ccw = False
        self._hold_button_clicked = False
        self._hold_button_released = False

        self._pub_delta = 0
        self._pub_change = 0
        self._pub_moved = False
        self._pub_cw = False
        self._pub_ccw = False
        self._pub_button_clicked = False
        self._pub_button_released = False

        self._button_state = 1
        self._last_button_state = 1
        self._button_clicked = False
        self._button_released = False
        self._last_button_change_ms = 0

        self._clk = None
        self._dt = None
        self._sw = None

    def begin(self):
        self._clk = _PinAdapter(self._clk_pin)
        self._dt = _PinAdapter(self._dt_pin)
        self._sw = _PinAdapter(self._sw_pin)

        self._last_encoded = self._read_encoded()
        self._count = 0
        self._edge_delta = 0
        self._raw_accum = 0
        self._edge_change = 0
        self._edge_moved = False
        self._edge_cw = False
        self._edge_ccw = False

        self._tx_open = False
        self._hold_delta = 0
        self._hold_change = 0
        self._hold_moved = False
        self._hold_cw = False
        self._hold_ccw = False
        self._hold_button_clicked = False
        self._hold_button_released = False
        self._pub_delta = 0
        self._pub_change = 0
        self._pub_moved = False
        self._pub_cw = False
        self._pub_ccw = False
        self._pub_button_clicked = False
        self._pub_button_released = False

        self._button_state = self._sw.value()
        self._last_button_state = self._button_state
        self._button_clicked = False
        self._button_released = False
        self._last_button_change_ms = _ticks_ms()
        self._initialized = True

    def _read_encoded(self):
        clk = self._clk.value() & 1
        dt = self._dt.value() & 1
        return (clk << 1) | dt

    def _apply_step(self, step):
        self._raw_accum += step
        while self._raw_accum >= self.STEPS_PER_DETENT:
            self._raw_accum -= self.STEPS_PER_DETENT
            self._count += 1
            self._edge_delta += 1
            self._edge_change = 1
            self._edge_moved = True
            self._edge_cw = True
        while self._raw_accum <= -self.STEPS_PER_DETENT:
            self._raw_accum += self.STEPS_PER_DETENT
            self._count -= 1
            self._edge_delta -= 1
            self._edge_change = -1
            self._edge_moved = True
            self._edge_ccw = True

    def _decode(self):
        encoded = self._read_encoded()
        if encoded != self._last_encoded:
            index = ((self._last_encoded << 2) | encoded) & 0x0F
            step = self.STATE_TABLE[index]
            self._last_encoded = encoded
            if step != 0:
                self._apply_step(step)

    def _update_button(self):
        reading = self._sw.value()
        now = _ticks_ms()
        if reading != self._last_button_state:
            self._last_button_change_ms = now
        if _ticks_diff(now, self._last_button_change_ms) > self.BUTTON_DEBOUNCE_MS:
            if reading != self._button_state:
                self._button_state = reading
                if self._button_state == 0:
                    self._button_clicked = True
                else:
                    self._button_released = True
        self._last_button_state = reading

    def _merge_edges(self):
        if self._edge_change != 0:
            self._pub_change = self._edge_change
            self._edge_change = 0
        if self._edge_delta != 0:
            self._pub_delta += self._edge_delta
            self._edge_delta = 0
        if self._edge_moved:
            self._pub_moved = True
            self._edge_moved = False
        if self._edge_cw:
            self._pub_cw = True
            self._edge_cw = False
        if self._edge_ccw:
            self._pub_ccw = True
            self._edge_ccw = False
        if self._button_clicked:
            self._pub_button_clicked = True
            self._button_clicked = False
        if self._button_released:
            self._pub_button_released = True
            self._button_released = False

    def sample(self):
        if not self._initialized:
            self.begin()
        for _ in range(self.POLL_SAMPLES):
            self._decode()
        self._update_button()
        self._merge_edges()

    def _close_tx_if_expired(self):
        if self._tx_open and _ticks_diff(_ticks_ms(), self._tx_start_ms) >= self.TX_HOLD_MS:
            self._tx_open = False
            self._hold_delta = 0
            self._hold_change = 0
            self._hold_moved = False
            self._hold_cw = False
            self._hold_ccw = False
            self._hold_button_clicked = False
            self._hold_button_released = False

    def _open_tx_if_needed(self):
        self.sample()
        self._close_tx_if_expired()
        if self._tx_open:
            return
        self._tx_open = True
        self._tx_start_ms = _ticks_ms()
        self._hold_delta = self._pub_delta
        self._hold_change = self._pub_change
        self._hold_moved = self._pub_moved
        self._hold_cw = self._pub_cw
        self._hold_ccw = self._pub_ccw
        self._hold_button_clicked = self._pub_button_clicked
        self._hold_button_released = self._pub_button_released
        self._pub_delta = 0
        self._pub_change = 0
        self._pub_moved = False
        self._pub_cw = False
        self._pub_ccw = False
        self._pub_button_clicked = False
        self._pub_button_released = False

    def loop(self):
        self.sample()
        self._close_tx_if_expired()

    def get_count(self):
        self.sample()
        return self._count

    def set_count(self, count):
        self._count = int(count)
        self._raw_accum = 0

    def reset_count(self):
        self._count = 0
        self._edge_delta = 0
        self._raw_accum = 0
        self._edge_change = 0
        self._edge_moved = False
        self._edge_cw = False
        self._edge_ccw = False
        self._tx_open = False
        self._hold_delta = 0
        self._hold_change = 0
        self._hold_moved = False
        self._hold_cw = False
        self._hold_ccw = False
        self._pub_delta = 0
        self._pub_change = 0
        self._pub_moved = False
        self._pub_cw = False
        self._pub_ccw = False

    def get_delta(self):
        self._open_tx_if_needed()
        return self._hold_delta

    def get_change(self):
        self._open_tx_if_needed()
        return self._hold_change

    def turned_clockwise(self):
        self._open_tx_if_needed()
        return self._hold_cw

    def turned_counter_clockwise(self):
        self._open_tx_if_needed()
        return self._hold_ccw

    def has_moved(self):
        self._open_tx_if_needed()
        return self._hold_moved

    def is_button_pressed(self):
        self.sample()
        return self._button_state == 0

    def was_button_clicked(self):
        self._open_tx_if_needed()
        return self._hold_button_clicked

    def was_button_released(self):
        self._open_tx_if_needed()
        return self._hold_button_released
