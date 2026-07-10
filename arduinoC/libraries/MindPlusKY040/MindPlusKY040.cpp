#include "MindPlusKY040.h"

#if defined(ESP32)
portMUX_TYPE MindPlusKY040::_mux = portMUX_INITIALIZER_UNLOCKED;
#else
MindPlusKY040 *MindPlusKY040::_instance = nullptr;
#endif

const int8_t MindPlusKY040::STATE_TABLE[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0
};

MindPlusKY040::MindPlusKY040(uint8_t clkPin, uint8_t dtPin, uint8_t swPin)
    : _clkPin(clkPin),
      _dtPin(dtPin),
      _swPin(swPin),
      _initialized(false),
      _useInterrupt(false),
      _txOpen(false),
      _txStartMs(0),
      _count(0),
      _edgeDelta(0),
      _rawAccum(0),
      _edgeChange(0),
      _edgeMoved(false),
      _edgeCw(false),
      _edgeCcw(false),
      _lastEncoded(0),
      _holdDelta(0),
      _holdChange(0),
      _holdMoved(false),
      _holdCw(false),
      _holdCcw(false),
      _holdButtonClicked(false),
      _holdButtonReleased(false),
      _pubDelta(0),
      _pubChange(0),
      _pubMoved(false),
      _pubCw(false),
      _pubCcw(false),
      _pubButtonClicked(false),
      _pubButtonReleased(false),
      _buttonState(HIGH),
      _lastButtonState(HIGH),
      _buttonClicked(false),
      _buttonReleased(false),
      _lastButtonChangeMs(0) {}

#if !defined(ESP32)
void MindPlusKY040::isrHandler() {
    if (_instance) {
        _instance->decodeRotationCoreUnsafe();
    }
}
#endif

void MindPlusKY040::enterCritical() {
#if defined(ESP32)
    portENTER_CRITICAL(&_mux);
#else
    noInterrupts();
#endif
}

void MindPlusKY040::exitCritical() {
#if defined(ESP32)
    portEXIT_CRITICAL(&_mux);
#else
    interrupts();
#endif
}

uint8_t MindPlusKY040::readEncodedState() const {
    uint8_t clk = digitalRead(_clkPin) & 0x01;
    uint8_t dt = digitalRead(_dtPin) & 0x01;
    return (clk << 1) | dt;
}

void MindPlusKY040::applyStep(int8_t step) {
    _rawAccum += step;

    while (_rawAccum >= STEPS_PER_DETENT) {
        _rawAccum -= STEPS_PER_DETENT;
        _count++;
        _edgeDelta++;
        _edgeChange = 1;
        _edgeMoved = true;
        _edgeCw = true;
    }

    while (_rawAccum <= -STEPS_PER_DETENT) {
        _rawAccum += STEPS_PER_DETENT;
        _count--;
        _edgeDelta--;
        _edgeChange = -1;
        _edgeMoved = true;
        _edgeCcw = true;
    }
}

void MindPlusKY040::decodeRotationCoreUnsafe() {
    uint8_t encoded = readEncodedState();
    if (encoded != _lastEncoded) {
        uint8_t index = (_lastEncoded << 2) | encoded;
        int8_t step = STATE_TABLE[index & 0x0F];
        _lastEncoded = encoded;
        if (step != 0) {
            applyStep(step);
        }
    }
}

void MindPlusKY040::decodeRotationCore() {
    enterCritical();
    decodeRotationCoreUnsafe();
    exitCritical();
}

#if !defined(ESP32)
void MindPlusKY040::attachRotationInterrupts() {
    _useInterrupt = false;
    _instance = this;

    int clkIntr = digitalPinToInterrupt(_clkPin);
    if (clkIntr != NOT_AN_INTERRUPT) {
        attachInterrupt(clkIntr, isrHandler, CHANGE);
        _useInterrupt = true;
    }

    if (_dtPin != _clkPin) {
        int dtIntr = digitalPinToInterrupt(_dtPin);
        if (dtIntr != NOT_AN_INTERRUPT) {
            attachInterrupt(dtIntr, isrHandler, CHANGE);
            _useInterrupt = true;
        }
    }
}
#else
void MindPlusKY040::attachRotationInterrupts() {
    _useInterrupt = false;
}
#endif

void MindPlusKY040::mergeEdgesToPub() {
    enterCritical();
    if (_edgeChange != 0) {
        _pubChange = _edgeChange;
        _edgeChange = 0;
    }
    if (_edgeDelta != 0) {
        _pubDelta += _edgeDelta;
        _edgeDelta = 0;
    }
    if (_edgeMoved) {
        _pubMoved = true;
        _edgeMoved = false;
    }
    if (_edgeCw) {
        _pubCw = true;
        _edgeCw = false;
    }
    if (_edgeCcw) {
        _pubCcw = true;
        _edgeCcw = false;
    }
    exitCritical();

    if (_buttonClicked) {
        _pubButtonClicked = true;
        _buttonClicked = false;
    }
    if (_buttonReleased) {
        _pubButtonReleased = true;
        _buttonReleased = false;
    }
}

void MindPlusKY040::sampleHardware() {
    if (!_initialized) {
        begin();
    }

#if defined(ESP32)
    uint16_t samples = POLL_SAMPLES;
#else
    uint16_t samples = _useInterrupt ? POLL_SAMPLES_ISR : POLL_SAMPLES;
#endif

    for (uint16_t i = 0; i < samples; i++) {
        decodeRotationCore();
        if (POLL_INTERVAL_US > 0) {
            delayMicroseconds(POLL_INTERVAL_US);
        }
    }

    updateButton();
    mergeEdgesToPub();
}

void MindPlusKY040::closeTxIfExpired() {
    if (_txOpen && (millis() - _txStartMs) >= TX_HOLD_MS) {
        _txOpen = false;
        _holdDelta = 0;
        _holdChange = 0;
        _holdMoved = false;
        _holdCw = false;
        _holdCcw = false;
        _holdButtonClicked = false;
        _holdButtonReleased = false;
    }
}

void MindPlusKY040::openTxIfNeeded() {
    sampleHardware();
    closeTxIfExpired();

    if (_txOpen) {
        return;
    }

    _txOpen = true;
    _txStartMs = millis();

    _holdDelta = _pubDelta;
    _holdChange = _pubChange;
    _holdMoved = _pubMoved;
    _holdCw = _pubCw;
    _holdCcw = _pubCcw;
    _holdButtonClicked = _pubButtonClicked;
    _holdButtonReleased = _pubButtonReleased;

    _pubDelta = 0;
    _pubChange = 0;
    _pubMoved = false;
    _pubCw = false;
    _pubCcw = false;
    _pubButtonClicked = false;
    _pubButtonReleased = false;
}

void MindPlusKY040::begin() {
    pinMode(_clkPin, INPUT_PULLUP);
    pinMode(_dtPin, INPUT_PULLUP);
    pinMode(_swPin, INPUT_PULLUP);

    enterCritical();
    _lastEncoded = readEncodedState();
    _count = 0;
    _edgeDelta = 0;
    _rawAccum = 0;
    _edgeChange = 0;
    _edgeMoved = false;
    _edgeCw = false;
    _edgeCcw = false;
    exitCritical();

    _txOpen = false;
    _txStartMs = 0;
    _holdDelta = 0;
    _holdChange = 0;
    _holdMoved = false;
    _holdCw = false;
    _holdCcw = false;
    _holdButtonClicked = false;
    _holdButtonReleased = false;
    _pubDelta = 0;
    _pubChange = 0;
    _pubMoved = false;
    _pubCw = false;
    _pubCcw = false;
    _pubButtonClicked = false;
    _pubButtonReleased = false;

    attachRotationInterrupts();

    _buttonState = digitalRead(_swPin);
    _lastButtonState = _buttonState;
    _buttonClicked = false;
    _buttonReleased = false;
    _lastButtonChangeMs = millis();
    _initialized = true;
}

void MindPlusKY040::updateButton() {
    bool reading = digitalRead(_swPin);
    unsigned long now = millis();

    if (reading != _lastButtonState) {
        _lastButtonChangeMs = now;
    }

    if ((now - _lastButtonChangeMs) > BUTTON_DEBOUNCE_MS) {
        if (reading != _buttonState) {
            _buttonState = reading;
            if (_buttonState == LOW) {
                _buttonClicked = true;
            } else {
                _buttonReleased = true;
            }
        }
    }

    _lastButtonState = reading;
}

void MindPlusKY040::loop() {
    sampleHardware();
    closeTxIfExpired();
}

int32_t MindPlusKY040::getCount() {
    sampleHardware();
    enterCritical();
    int32_t value = _count;
    exitCritical();
    return value;
}

void MindPlusKY040::setCount(int32_t count) {
    enterCritical();
    _count = count;
    _rawAccum = 0;
    exitCritical();
}

void MindPlusKY040::resetCount() {
    enterCritical();
    _count = 0;
    _edgeDelta = 0;
    _rawAccum = 0;
    _edgeChange = 0;
    _edgeMoved = false;
    _edgeCw = false;
    _edgeCcw = false;
    exitCritical();

    _txOpen = false;
    _holdDelta = 0;
    _holdChange = 0;
    _holdMoved = false;
    _holdCw = false;
    _holdCcw = false;
    _pubDelta = 0;
    _pubChange = 0;
    _pubMoved = false;
    _pubCw = false;
    _pubCcw = false;
}

int32_t MindPlusKY040::getDelta() {
    openTxIfNeeded();
    return _holdDelta;
}

int8_t MindPlusKY040::getChange() {
    openTxIfNeeded();
    return _holdChange;
}

bool MindPlusKY040::turnedClockwise() {
    openTxIfNeeded();
    return _holdCw;
}

bool MindPlusKY040::turnedCounterClockwise() {
    openTxIfNeeded();
    return _holdCcw;
}

bool MindPlusKY040::hasMoved() {
    openTxIfNeeded();
    return _holdMoved;
}

bool MindPlusKY040::isButtonPressed() {
    sampleHardware();
    if (!_initialized) {
        return false;
    }
    return _buttonState == LOW;
}

bool MindPlusKY040::wasButtonClicked() {
    openTxIfNeeded();
    return _holdButtonClicked;
}

bool MindPlusKY040::wasButtonReleased() {
    openTxIfNeeded();
    return _holdButtonReleased;
}
