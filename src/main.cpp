#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>
#include <Button2.h>
#include <SPI.h>
#include <Wire.h>
#include <config.h>

Button2 mainButton;
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_DotStar dot(1, DOTSTAR_DATA_PIN, DOTSTAR_CLOCK_PIN, DOTSTAR_BRG);

// Global variables
enum pomodoroMode
{
    INACTIVE,
    WORK,
    REST,
    BREAK,
    SLEEP,
    PAUSE
};
long pomodoroStartMs;
int pomodoroMode = INACTIVE;
int pomodoroCycle = 0;

u_long vibrationLastPulseMs;
int vibrationPinState = LOW;
bool isVibrationPulsing = false;

// Function definitions
void initializePixels();
void advancePomodoroMode(u_long ms);
void handleButtonPress(Button2 &btn);
void updateAnimation(u_long ms);
void chase(int colors[3]);
void clock(int colors[3], long elapsedMs, float totalMins);
void breath(int colors[3]);
void startVibrationPulse();
void loopVibrationPulse();
void stopVibrationPulse();

void setup()
{
    mainButton.setLongClickTime(1000);
    mainButton.begin(MAIN_BUTTON_PIN, INPUT_PULLDOWN, false);
    mainButton.setClickHandler(handleButtonPress);
    mainButton.setLongClickDetectedHandler(handleButtonPress);
    mainButton.setDoubleClickHandler(handleButtonPress);
    mainButton.setTripleClickHandler(handleButtonPress);

    pinMode(VIBRATION_PIN, OUTPUT);

    pixels.begin();
    Serial.begin(115200);

    delay(100);

    initializePixels();

    dot.begin();
    dot.setPixelColor(0, 0, 0, 5);
    dot.show();
}

u_long currentMs;
u_long lastLoop = 0;

void loop()
{
    currentMs = millis();

    mainButton.loop();

    if (currentMs - lastLoop > 10)
    {
        updateAnimation(currentMs);
        loopVibrationPulse();

        pixels.setBrightness(64);
        pixels.show();

        lastLoop = currentMs;
    }
}

int pauseResumeMode;
u_long pauseElapsedMs;

void handleButtonPress(Button2 &btn)
{
    switch (btn.getType())
    {
    case single_click:
        advancePomodoroMode(currentMs);
        break;
    case double_click:
        if (pomodoroMode != PAUSE)
        {
            stopVibrationPulse();
            pauseResumeMode = pomodoroMode;
            pauseElapsedMs = currentMs - pomodoroStartMs;
            pomodoroMode = PAUSE;
            pixels.clear();
        }
        else
        {
            advancePomodoroMode(currentMs);
        }
        break;
    case triple_click:
        // TODO: Use this to show battery life?
        break;
    case long_click:
        stopVibrationPulse();
        pomodoroMode = pomodoroMode == SLEEP ? INACTIVE : SLEEP;
        pomodoroCycle = 1;

        if (pomodoroMode == SLEEP)
        {
            dot.setPixelColor(0, 0, 0, 0);
            dot.show();
            pixels.clear();
            pixels.show();
        }
        else
        {
            dot.setPixelColor(0, 0, 0, 5);
            dot.show();
        }
        break;
    case empty:
        return;
    }
}

void advancePomodoroMode(u_long ms)
{
    int currentMode = pomodoroMode;

    stopVibrationPulse();

    switch (currentMode)
    {
    case PAUSE:
        pomodoroMode = pauseResumeMode;
        pomodoroStartMs = currentMs - pauseElapsedMs;
        break;
    case INACTIVE:
        pomodoroMode = WORK;
        pomodoroStartMs = ms;
        break;
    case WORK:
        pomodoroMode = pomodoroCycle >= POMODORO_CYCLES - 1 ? BREAK : REST;
        pomodoroStartMs = ms;
        break;
    case REST:
        pomodoroMode = WORK;
        pomodoroStartMs = ms;
        ++pomodoroCycle;
        break;
    case BREAK:
        pomodoroMode = WORK;
        pomodoroStartMs = ms;
        pomodoroCycle = 1;
    default:
        break;
    }
}

void initializePixels()
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        pixels.setPixelColor(i, 0);
    }

    pixels.show();
}

int colors[3];

void updateAnimation(u_long ms)
{
    long elapsedMs = (ms - pomodoroStartMs);
    bool hasExceededTime = false;

    switch (pomodoroMode)
    {
    case INACTIVE:
        colors[0] = 255;
        colors[1] = 202;
        colors[2] = 58;
        breath(colors);
        break;
    case WORK:
        colors[0] = 255;
        colors[1] = 89;
        colors[2] = 94;
        hasExceededTime = elapsedMs > POMODORO_WORK_MINS * 60000;
        if (hasExceededTime)
        {
            if (!isVibrationPulsing)
            {
                startVibrationPulse();
            }
            breath(colors);
        }
        else
        {
            clock(colors, elapsedMs, POMODORO_WORK_MINS);
        }
        break;
    case REST:
        colors[0] = 25;
        colors[1] = 130;
        colors[2] = 196;
        hasExceededTime = elapsedMs > POMODORO_REST_MINS * 60000;
        if (hasExceededTime)
        {
            if (!isVibrationPulsing)
            {
                startVibrationPulse();
            }
            breath(colors);
        }
        else
        {
            clock(colors, elapsedMs, POMODORO_REST_MINS);
        }
        break;
    case BREAK:
        colors[0] = 138;
        colors[1] = 201;
        colors[2] = 38;
        hasExceededTime = elapsedMs > POMODORO_BREAK_MINS * 60000;
        if (hasExceededTime)
        {
            if (!isVibrationPulsing)
            {
                startVibrationPulse();
            }
            breath(colors);
        }
        else
        {
            clock(colors, elapsedMs, POMODORO_BREAK_MINS);
        }
        break;
    case PAUSE:
        chase(colors);
        break;
    default:
        break;
    }
}

long lastMoveMs = -1;
int nextPosition = 0;

void chase(int colors[3])
{
    u_long currentMs = millis();

    if (currentMs - lastMoveMs > CHASE_DELAY_MS)
    {
        // Turn off the tail pixel that's furthest back
        int tailEndIndex = (nextPosition - CHASE_TAIL_LENGTH + NUM_PIXELS) % NUM_PIXELS;
        pixels.setPixelColor(tailEndIndex, 0);

        // Set the current head pixel
        pixels.setPixelColor(nextPosition, pixels.Color(colors[0], colors[1], colors[2]));

        // Update the tail pixels
        for (int i = 1; i < CHASE_TAIL_LENGTH; ++i)
        {
            int tailIndex = (nextPosition - i + NUM_PIXELS) % NUM_PIXELS;
            float scale = 1.0 * (CHASE_TAIL_LENGTH - i) / CHASE_TAIL_LENGTH;

            // Precompute scaled colors
            uint8_t scaledR = colors[0] * scale;
            uint8_t scaledG = colors[1] * scale;
            uint8_t scaledB = colors[2] * scale;

            pixels.setPixelColor(tailIndex, pixels.Color(scaledR, scaledG, scaledB));
        }

        // Update position and timing
        lastMoveMs = currentMs;
        nextPosition = (nextPosition + 1) % NUM_PIXELS;
    }
}

long clockLastMoveMs = -1;
long clockPulseStep = 0;
float clockPulseSteps = CLOCK_PULSE_MS / CLOCK_DELAY_MS;

void clock(int colors[3], long elapsedMs, float totalMins)
{
    long clockMs = millis();
    float timePerPixelMs = (1.0f * totalMins / NUM_PIXELS) * 60000.0;

    if (clockMs - clockLastMoveMs > CLOCK_DELAY_MS)
    {
        pixels.clear();

        int lastPixelIndex = ceil(elapsedMs / timePerPixelMs);

        for (int i = 0; i < lastPixelIndex; ++i)
        {
            float scale = 0.1;
            float scaledR = colors[0] * scale;
            float scaledG = colors[1] * scale;
            float scaledB = colors[2] * scale;

            pixels.setPixelColor(i, pixels.Color(scaledR, scaledG, scaledB));
        }

        // Pulse the lead pixel
        float pulseScale = abs(clockPulseStep - (CLOCK_PULSE_MS / 2.0)) / (CLOCK_PULSE_MS / 2.0);
        float pulsedR = colors[0] * pulseScale;
        float pulsedG = colors[1] * pulseScale;
        float pulsedB = colors[2] * pulseScale;

        pixels.setPixelColor(lastPixelIndex - 1, pixels.Color(pulsedR, pulsedG, pulsedB));

        // Prepare next step
        clockPulseStep = clockPulseStep + CLOCK_DELAY_MS > CLOCK_PULSE_MS ? 0 : clockPulseStep + CLOCK_DELAY_MS;
        clockLastMoveMs = clockMs;
    }
}

long breathLastMoveMs = -1;
long breathPulseStep = 0;
float breathPulseSteps = CLOCK_PULSE_MS / CLOCK_DELAY_MS;

void breath(int colors[3])
{
    long clockMs = currentMs;

    if (clockMs - breathLastMoveMs > CLOCK_DELAY_MS)
    {
        pixels.clear();
        float pulseScale = abs(breathPulseStep - (CLOCK_PULSE_MS / 2.0)) / (CLOCK_PULSE_MS / 2.0);
        float pulsedR = colors[0] * pulseScale;
        float pulsedG = colors[1] * pulseScale;
        float pulsedB = colors[2] * pulseScale;

        for (int i = 0; i < NUM_PIXELS; ++i)
        {
            pixels.setPixelColor(i, pixels.Color(pulsedR, pulsedG, pulsedB));
        }

        // Prepare next step
        breathPulseStep = breathPulseStep + CLOCK_DELAY_MS > CLOCK_PULSE_MS ? 0 : breathPulseStep + CLOCK_DELAY_MS;
        breathLastMoveMs = clockMs;
    }
}

void startVibrationPulse()
{
    vibrationLastPulseMs = currentMs;
    isVibrationPulsing = true;
    vibrationPinState = HIGH;
}

void loopVibrationPulse()
{
    digitalWrite(VIBRATION_PIN, vibrationPinState);

    if (isVibrationPulsing && currentMs - vibrationLastPulseMs >= VIBRATION_PULSE_MS)
    {
        vibrationPinState = vibrationPinState == HIGH ? LOW : HIGH;
        vibrationLastPulseMs = currentMs;
    }
}

void stopVibrationPulse()
{
    isVibrationPulsing = false;
    vibrationPinState = LOW;
}