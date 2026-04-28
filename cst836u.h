#pragma once
#include <Arduino.h>
#include <Wire.h>

class CST836U {
public:
    enum Gesture {
        NONE = 0,
        TAP,
        DOUBLE_TAP,
        LONG_PRESS,
        SWIPE_LEFT,
        SWIPE_RIGHT,
        SWIPE_UP,
        SWIPE_DOWN
    };

    struct Config {
        uint16_t rawWidth = 240;
        uint16_t rawHeight = 320;

        uint16_t swipeThreshold = 50;
        uint16_t tapMaxTime = 200;
        uint16_t doubleTapTime = 300;
        uint16_t longPressTime = 600;

        uint16_t tapDistance = 20;
        uint16_t longPressMaxMove = 10;
    };

    struct TouchEvent {
        uint16_t x;
        uint16_t y;
        bool touched;
        bool pressed;   // touch start
        bool released;  // touch end
        bool moved;
        Gesture gesture;
    };

public:
    CST836U(TwoWire &wire = Wire);

    void begin(Config cfg, int sda, int scl, int rst, int irq);
    void setRotation(uint8_t rotation);
    void enableInterrupt(uint8_t pin);
    bool isInterruptTriggered();
    bool update(TouchEvent &event);

    uint16_t width() const { return _width; }
    uint16_t height() const { return _height; }

private:
    bool readRaw(uint16_t &x, uint16_t &y);
    void applyRotation(uint16_t &x, uint16_t &y);
    static void IRAM_ATTR isrHandler(void* arg);
    volatile bool _irqFlag = false;
    uint8_t _irqPin = 255;

private:
    TwoWire *_wire;
    Config _cfg;

    uint16_t _width, _height;
    uint8_t _rotation = 0;

    // state
    bool _active = false;
    bool _longPressFired = false;
    bool _tapPending = false;

    uint32_t _startTime = 0;
    uint32_t _lastTapTime = 0;

    uint16_t _startX, _startY;
    uint16_t _lastX, _lastY;

    uint16_t _lastTapX = 0;
    uint16_t _lastTapY = 0;
};