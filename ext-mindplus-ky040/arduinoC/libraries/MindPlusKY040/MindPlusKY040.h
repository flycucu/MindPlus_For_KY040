#ifndef MIND_PLUS_KY040_H
#define MIND_PLUS_KY040_H

#include <Arduino.h>

class MindPlusKY040 {
public:
    MindPlusKY040(uint8_t clkPin, uint8_t dtPin, uint8_t swPin);

    void begin();
    void loop();

    int32_t getCount();
    void setCount(int32_t count);
    void resetCount();

    int32_t getDelta();
    int8_t getChange();
    bool turnedClockwise();
    bool turnedCounterClockwise();
    bool hasMoved();

    bool isButtonPressed();
    bool wasButtonClicked();
    bool wasButtonReleased();

private:
#if defined(ESP32)
    static portMUX_TYPE _mux;
#else
    static MindPlusKY040 *_instance;
    static void isrHandler();
#endif

    uint8_t _clkPin;
    uint8_t _dtPin;
    uint8_t _swPin;

    bool _initialized;
    bool _useInterrupt;
    bool _txOpen;
    unsigned long _txStartMs;

    volatile int32_t _count;
    volatile int32_t _edgeDelta;
    volatile int8_t _rawAccum;
    volatile int8_t _edgeChange;
    volatile bool _edgeMoved;
    volatile bool _edgeCw;
    volatile bool _edgeCcw;
    volatile uint8_t _lastEncoded;

    int32_t _holdDelta;
    int8_t _holdChange;
    bool _holdMoved;
    bool _holdCw;
    bool _holdCcw;
    bool _holdButtonClicked;
    bool _holdButtonReleased;

    int32_t _pubDelta;
    int8_t _pubChange;
    bool _pubMoved;
    bool _pubCw;
    bool _pubCcw;
    bool _pubButtonClicked;
    bool _pubButtonReleased;

    bool _buttonState;
    bool _lastButtonState;
    bool _buttonClicked;
    bool _buttonReleased;
    unsigned long _lastButtonChangeMs;

    static const unsigned long BUTTON_DEBOUNCE_MS = 50;
    static const unsigned long TX_HOLD_MS = 20;
    static const int8_t STEPS_PER_DETENT = 4;
#if defined(ESP32)
    static const uint16_t POLL_SAMPLES = 64;
    static const uint16_t POLL_INTERVAL_US = 200;
#else
    static const uint16_t POLL_SAMPLES = 200;
    static const uint16_t POLL_INTERVAL_US = 50;
    static const uint16_t POLL_SAMPLES_ISR = 48;
#endif

    static const int8_t STATE_TABLE[16];

    void enterCritical();
    void exitCritical();
    uint8_t readEncodedState() const;
    void applyStep(int8_t step);
    void decodeRotationCoreUnsafe();
    void decodeRotationCore();
    void attachRotationInterrupts();
    void mergeEdgesToPub();
    void sampleHardware();
    void closeTxIfExpired();
    void openTxIfNeeded();
    void updateButton();
};

#endif
