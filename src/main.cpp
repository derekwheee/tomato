#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
// This lib is fine for now but there are better ones
#include <Button.h>
#include <config.h>

Button mainButton(MAIN_BUTTON_PIN);
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRBW + NEO_KHZ800);

// Global variables
enum pomodoroMode
{
    INACTIVE,
    WORK,
    REST,
    BREAK
};
u_long pomodoroStartMs;
int pomodoroMode = INACTIVE;
int pomodoroCycle = 0;
int lastPixel = NUM_PIXELS;

// Function definitions
void initializePixels();
void updatePomodoroCycle(u_long ms);
void updateLedRing(u_long ms);
void handleButtonPress();
void updateAnimation(u_long ms);
float breath(int value, int timeFactor = 1);

void setup()
{
    mainButton.begin();
    pixels.begin();

    Serial.begin(115200);

    delay(100);

    initializePixels();
}

void loop()
{
    u_long currentMs = millis();
    // initializePixels();

    if (mainButton.released())
    {
        Serial.println("Button released");
        handleButtonPress();
    }

    if (pomodoroMode != INACTIVE)
    {
        updatePomodoroCycle(currentMs);
        updateLedRing(currentMs);
    }

    updateAnimation(currentMs);

    pixels.show();
}

void handleButtonPress()
{
    if (pomodoroMode == INACTIVE)
    {
        pomodoroMode = WORK;
        pomodoroStartMs = 0;
    }
    else
    {
        // This will force `updatePomodoroCycle` to the next cycle
        pomodoroStartMs = -999999;
    }
}

void updatePomodoroCycle(u_long ms)
{
    u_long elapsedMs = ms - pomodoroStartMs;

    if (pomodoroMode == WORK && elapsedMs > POMODORO_WORK_MINS * 60000)
    {
        pomodoroMode = pomodoroCycle >= POMODORO_CYCLES ? BREAK : REST;
        pomodoroStartMs = ms;
    }
    else if (pomodoroMode == REST && elapsedMs > POMODORO_REST_MINS * 60000)
    {
        pomodoroMode = WORK;
        pomodoroStartMs = ms;
        ++pomodoroCycle;
    }
    else if (pomodoroMode == BREAK && elapsedMs > POMODORO_BREAK_MINS * 60000)
    {
        pomodoroMode = WORK;
        pomodoroStartMs = ms;
        pomodoroCycle = 1;
    }
}

void updateLedRing(u_long ms)
{
    u_long elapsedMs = ms - pomodoroStartMs;
    long factor;

    if (pomodoroMode == WORK)
    {
        factor = POMODORO_WORK_MINS * 60000 / NUM_PIXELS;
        lastPixel = floor(elapsedMs / factor);
    }
    else if (pomodoroMode == REST)
    {
        factor = POMODORO_REST_MINS * 60000 / NUM_PIXELS;
        lastPixel = floor(elapsedMs / factor);
    }
    else if (pomodoroMode == BREAK)
    {
        factor = POMODORO_BREAK_MINS * 60000 / NUM_PIXELS;
        lastPixel = floor(elapsedMs / factor);
    }
    else
    {
        // Fail state
    }

    lastPixel = lastPixel == 0 ? 1 : lastPixel;
}

void initializePixels()
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        pixels.setPixelColor(i, pixels.Color(0, (1.0 * i / NUM_PIXELS) * 50, (1.0 * i / NUM_PIXELS) * 50, 0));
    }

    pixels.show();
}

void updateAnimation(u_long ms)
{
    pixels.clear();

    if (pomodoroMode == INACTIVE)
    {
        for (int i = 0; i < NUM_PIXELS; i++)
        {
            pixels.setPixelColor(i, pixels.Color(0, breath(50), breath(50), 0));
        }
    }
    else if (pomodoroMode == WORK)
    {
        for (int i = 0; i < lastPixel; i++)
        {
            pixels.setPixelColor(i, pixels.Color(breath(50), 0, 0, 0));
        }
    }
    else if (pomodoroMode == REST)
    {
        for (int i = 0; i < lastPixel; i++)
        {
            pixels.setPixelColor(i, pixels.Color(0, 0, breath(50), 0));
        }
    }
    else if (pomodoroMode == BREAK)
    {
        for (int i = 0; i < lastPixel; i++)
        {
            pixels.setPixelColor(i, pixels.Color(0, breath(50), 0, 0));
        }
    }
}

int brightness = 0;
int direction = 1;

float breath(int value, int timeFactor)
{
    int range = 32768 * timeFactor;

    brightness = brightness + direction;
    if (brightness >= range)
    {
        direction = -1;
    }
    if (brightness <= 0)
    {
        direction = 1;
    }

    float scalar = 1.0 * brightness / range;

    return value * scalar;
}