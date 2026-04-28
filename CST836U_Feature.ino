#include <TFT_eSPI.h>
#include "cst836u.h"

TFT_eSPI tft = TFT_eSPI();
CST836U touch;

#define TOUCH_SDA 4
#define TOUCH_SCL 5
#define TOUCH_RST 2
#define TOUCH_INT 3

uint8_t rotation = 1;
unsigned long lastRotate = 0;

bool drawing = false;
int lastX = 0;
int lastY = 0;

void drawUI() {
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 10);
    tft.println("CST836U FULL TEST");

    tft.println("----------------");
    tft.println("Tap: draw dot");
    tft.println("Swipe: change color");
    tft.println("Long press: clear");
    tft.println("Auto rotate: every 5s");
}

void setup() {
    Serial.begin(115200);

    tft.init();
    tft.setRotation(rotation);

    CST836U::Config cfg;
    cfg.rawWidth = 240;
    cfg.rawHeight = 320;

    touch.begin(cfg, TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);
    touch.setRotation(rotation);
    touch.enableInterrupt(TOUCH_INT);

    drawUI();
}

uint16_t color = TFT_GREEN;

void loop() {
    CST836U::TouchEvent e;

    // =========================
    // TOUCH UPDATE
    // =========================
    if (touch.update(e)) {

        tft.setCursor(80, 80);
        tft.print("X-");
        tft.print(e.x);
        tft.print(" Y-");
        tft.print(e.y);
        tft.print(" G-");
        tft.println(e.gesture);
        

        // ===== SERIAL DEBUG =====
        /*Serial.print("X:");
        Serial.print(e.x);
        Serial.print(" Y:");
        Serial.print(e.y);
        Serial.print(" T:");
        Serial.print(e.touched);
        Serial.print(" G:");
        Serial.println(e.gesture);*/

        // =========================
        // TOUCH STATES
        // =========================
        if (e.pressed) {
            //Serial.println("Pressed");
            drawing = true;
            lastX = e.x;
            lastY = e.y;
        }

        if (e.moved) {
            //tft.drawPixel(e.x, e.y, TFT_YELLOW);
            if (e.touched && drawing) {

                // draw continuous line
                tft.drawLine(lastX, lastY, e.x, e.y, TFT_WHITE);

                // update last point
                lastX = e.x;
                lastY = e.y;
            }
        }

        if (e.released) {
            //Serial.println("Released");
            drawing = false;
        }

        // =========================
        // GESTURES
        // =========================
        switch (e.gesture) {

            case CST836U::TAP:
                tft.fillCircle(e.x, e.y, 3, color);
                tft.setCursor(e.x, e.y - 15);
                tft.print("T ");
                tft.print(e.x);
                tft.print(" ");
                tft.println(e.y);
                //Serial.println("TAP");
                break;

            case CST836U::DOUBLE_TAP:
                tft.fillCircle(e.x, e.y, 5, TFT_RED);
                tft.setCursor(e.x, e.y - 20);
                tft.print("D ");
                tft.print(e.x);
                tft.print(" ");
                tft.println(e.y);
                //Serial.println("DOUBLE TAP");
                break;

            case CST836U::LONG_PRESS:
                tft.fillScreen(TFT_BLACK);
                drawUI();
                //Serial.println("LONG PRESS (CLEAR)");
                break;

            case CST836U::SWIPE_LEFT:
                color = TFT_BLUE;
                //Serial.println("SWIPE LEFT");
                break;

            case CST836U::SWIPE_RIGHT:
                color = TFT_GREEN;
                //Serial.println("SWIPE RIGHT");
                break;

            case CST836U::SWIPE_UP:
                color = TFT_YELLOW;
                //Serial.println("SWIPE UP");
                break;

            case CST836U::SWIPE_DOWN:
                color = TFT_WHITE;
                //Serial.println("SWIPE DOWN");
                break;

            default:
                break;
        }
    }

    // =========================
    // AUTO ROTATION (SIMULATE IMU)
    // =========================
    if (millis() - lastRotate > 5000) {
        lastRotate = millis();

        //rotation = (rotation + 1) % 4;
        rotation = rotation % 4;
        tft.setRotation(rotation);
        touch.setRotation(rotation);

        drawUI();

        //Serial.print("Rotation changed: ");
        //Serial.println(rotation);
    }
}