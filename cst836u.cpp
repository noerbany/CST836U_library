#include "cst836u.h"

#define CST_ADDR 0x15

CST836U::CST836U(TwoWire &wire) {
    _wire = &wire;
}

void CST836U::begin(Config cfg, int sda, int scl, int rst, int irq) {
    _cfg = cfg;

    _wire->begin(sda, scl);
    _wire->setClock(400000);

    pinMode(rst, OUTPUT);
    digitalWrite(rst, LOW);
    delay(10);
    digitalWrite(rst, HIGH);
    delay(50);

    pinMode(irq, INPUT_PULLUP);

    setRotation(0);
}

void CST836U::enableInterrupt(uint8_t pin) {
    _irqPin = pin;
    pinMode(_irqPin, INPUT_PULLUP);

    attachInterruptArg(
        digitalPinToInterrupt(_irqPin),
        isrHandler,
        this,
        FALLING   // CST836U triggers LOW
    );
}

void IRAM_ATTR CST836U::isrHandler(void* arg) {
    CST836U* self = static_cast<CST836U*>(arg);
    self->_irqFlag = true;
}

bool CST836U::isInterruptTriggered() {
    return _irqFlag;
}

void CST836U::setRotation(uint8_t r) {
    _rotation = r % 4;

    if (_rotation % 2 == 0) {
        _width  = _cfg.rawWidth;
        _height = _cfg.rawHeight;
    } else {
        _width  = _cfg.rawHeight;
        _height = _cfg.rawWidth;
    }
}

bool CST836U::readRaw(uint16_t &x, uint16_t &y) {
    uint8_t buf[7];

    _wire->beginTransmission(CST_ADDR);
    _wire->write(0x00);
    if (_wire->endTransmission(false) != 0) return false;

    _wire->requestFrom(CST_ADDR, 7);

    for (int i = 0; i < 7; i++) {
        if (_wire->available()) buf[i] = _wire->read();
    }

    if ((buf[2] & 0x0F) > 0) {
        x = ((buf[3] & 0x0F) << 8) | buf[4];
        y = ((buf[5] & 0x0F) << 8) | buf[6];
        return true;
    }

    return false;
}

void CST836U::applyRotation(uint16_t &x, uint16_t &y) {
    uint16_t tx = x, ty = y;

    switch (_rotation) {
        case 1:
            x = ty;
            y = _cfg.rawWidth - tx;
            break;
        case 2:
            x = _cfg.rawWidth - tx;
            y = _cfg.rawHeight - ty;
            break;
        case 3:
            x = _cfg.rawHeight - ty;
            y = tx;
            break;
    }
}

bool CST836U::update(TouchEvent &e) {

    // If interrupt enabled → only read when triggered
    if (_irqPin != 255) {
        if (!_irqFlag) return false;
        _irqFlag = false;
    }

    uint16_t x = _lastX;
    uint16_t y = _lastY;

    bool touched = readRaw(x, y);
    uint32_t now = millis();

    if (touched) {
        applyRotation(x, y);
        _lastX = x;
        _lastY = y;
    }
    e = {};
    e.x = x;
    e.y = y;
    e.touched = touched;

    // TOUCH START
    if (touched && !_active) {
        _active = true;
        _startTime = now;
        _startX = x;
        _startY = y;
        _lastX = x;
        _lastY = y;

        _longPressFired = false;

        e.pressed = true;
    }

    // MOVE
    if (touched && _active) {
        int moveX = abs(x - _startX);
        int moveY = abs(y - _startY);

        bool isStill = (moveX < _cfg.longPressMaxMove &&
                        moveY < _cfg.longPressMaxMove);

        if (!_longPressFired &&
            isStill &&
            (now - _startTime > _cfg.longPressTime)) {

            _longPressFired = true;
            e.gesture = LONG_PRESS;
        } else{
            e.moved = true;
        }
    }

    // RELEASE
    if (!touched && _active) {
        _active = false;
        e.released = true;

        int dx = _lastX - _startX;
        int dy = _lastY - _startY;
        uint32_t duration = now - _startTime;

        // SWIPE
        if (abs(dx) > _cfg.swipeThreshold ||
            abs(dy) > _cfg.swipeThreshold) {

            if (abs(dx) > abs(dy)) {
                e.gesture = dx > 0 ? SWIPE_RIGHT : SWIPE_LEFT;
            } else {
                e.gesture = dy > 0 ? SWIPE_DOWN : SWIPE_UP;
            }
            return true;
        }

        // TAP / DOUBLE TAP
        if (duration < _cfg.tapMaxTime && !_longPressFired) {

          // check same area
          bool sameArea = abs(_startX - _lastTapX) < _cfg.tapDistance &&
                          abs(_startY - _lastTapY) < _cfg.tapDistance;

          if (_tapPending &&
              (now - _lastTapTime < _cfg.doubleTapTime) &&
              sameArea) {

              _tapPending = false;
              e.gesture = DOUBLE_TAP;
              return true;
          }
          else{
            // fire TAP immediately (no delay)
            e.gesture = TAP;
          }

          _tapPending = true;
          _lastTapTime = now;
          _lastTapX = _startX;
          _lastTapY = _startY;

          return true;
      }
    }

    // SINGLE TAP (delayed)
    if (_tapPending &&
        (now - _lastTapTime > _cfg.doubleTapTime)) {

        _tapPending = false;
        e.gesture = TAP;
        return true;
    }

    return touched;
}