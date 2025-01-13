#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Button2.h>
#include <config.h>

Button2 mainButton;
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRBW + NEO_KHZ800);

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
u_long pomodoroStartMs;
int pomodoroMode = INACTIVE;
int pomodoroCycle = 0;
int lastPixel = NUM_PIXELS;

// Function definitions
void initializePixels();
void updatePomodoroCycle(u_long ms);
void updateLedRing(u_long ms);
void handleButtonPress(Button2 &btn);
void updateAnimation(u_long ms);
void chase(int colors[4]);
void clock(int colors[4], long elapsedMs, int totalMins);

void setup()
{
    mainButton.setLongClickTime(1000);
    mainButton.begin(MAIN_BUTTON_PIN);
    mainButton.setClickHandler(handleButtonPress);
    mainButton.setLongClickDetectedHandler(handleButtonPress);
    mainButton.setDoubleClickHandler(handleButtonPress);
    mainButton.setTripleClickHandler(handleButtonPress);

    pixels.begin();
    Serial.begin(115200);

    delay(100);

    initializePixels();
}

u_long currentMs;
u_long lastLoop = 0;

void loop()
{
    currentMs = millis();

    mainButton.loop();

    if (currentMs - lastLoop > 10)
    {
        if (pomodoroMode != INACTIVE && pomodoroMode != SLEEP && pomodoroMode != PAUSE)
        {
            updatePomodoroCycle(currentMs);
            updateLedRing(currentMs);
        }

        updateAnimation(currentMs);

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
        if (pomodoroMode == INACTIVE)
        {
            pomodoroMode = WORK;
            pomodoroStartMs = currentMs;
        }
        else if (pomodoroMode == PAUSE)
        {
            pomodoroMode = pauseResumeMode;
            pomodoroStartMs = currentMs - pauseElapsedMs;
        }
        else
        {
            // This will force `updatePomodoroCycle` to the next cycle
            pomodoroStartMs = -999999999;
        }
        break;
    case double_click:
        pauseResumeMode = pomodoroMode;
        pauseElapsedMs = currentMs - pomodoroStartMs;
        pomodoroMode = PAUSE;
        pixels.clear();
        break;
    case triple_click:
        Serial.print("triple ");
        break;
    case long_click:
        pomodoroMode = pomodoroMode == SLEEP ? INACTIVE : SLEEP;

        if (pomodoroMode == SLEEP)
        {
            pixels.clear();
            pixels.show();
        }
        break;
    case empty:
        return;
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

    switch (pomodoroMode)
    {
    case WORK:
        factor = POMODORO_WORK_MINS * 60000 / NUM_PIXELS;
        lastPixel = floor(elapsedMs / factor);
        break;
    case REST:
        factor = POMODORO_REST_MINS * 60000 / NUM_PIXELS;
        lastPixel = floor(elapsedMs / factor);
        break;
    case BREAK:
        factor = POMODORO_BREAK_MINS * 60000 / NUM_PIXELS;
        lastPixel = floor(elapsedMs / factor);
        break;
    default:
        break;
    }

    lastPixel = lastPixel == 0 ? 1 : lastPixel;
}

void initializePixels()
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        pixels.setPixelColor(i, 0);
    }

    pixels.show();
}

void updateAnimation(u_long ms)
{
    long elapsedMs = (ms - pomodoroStartMs);
    int colors[4];

    switch (pomodoroMode)
    {
    case INACTIVE:
        colors[0] = 0;
        colors[1] = 0;
        colors[2] = 0;
        colors[3] = 100;
        chase(colors);
        break;
    case WORK:
        colors[0] = 100;
        colors[1] = 0;
        colors[2] = 20;
        colors[3] = 0;
        clock(colors, elapsedMs, POMODORO_WORK_MINS);
        break;
    case REST:
        colors[0] = 0;
        colors[1] = 20;
        colors[2] = 100;
        colors[3] = 0;
        clock(colors, elapsedMs, POMODORO_REST_MINS);
        break;
    case BREAK:
        colors[0] = 0;
        colors[1] = 100;
        colors[2] = 20;
        colors[3] = 0;
        clock(colors, elapsedMs, POMODORO_BREAK_MINS);
        break;
    case PAUSE:
        colors[0] = 20;
        colors[1] = 20;
        colors[2] = 0;
        colors[3] = 0;
        chase(colors);
        break;
    default:
        break;
    }
}

long lastMoveMs = -1;
int nextPosition = 0;

void chase(int colors[4])
{
    u_long currentMs = millis();

    if (currentMs - lastMoveMs > CHASE_DELAY_MS)
    {
        // Turn off the tail pixel that's furthest back
        int tailEndIndex = (nextPosition - CHASE_TAIL_LENGTH + NUM_PIXELS) % NUM_PIXELS;
        pixels.setPixelColor(tailEndIndex, 0);

        // Set the current head pixel
        pixels.setPixelColor(nextPosition, pixels.Color(colors[0], colors[1], colors[2], colors[3]));

        // Update the tail pixels
        for (int i = 1; i < CHASE_TAIL_LENGTH; ++i)
        {
            int tailIndex = (nextPosition - i + NUM_PIXELS) % NUM_PIXELS;
            float scale = 1.0 * (CHASE_TAIL_LENGTH - i) / CHASE_TAIL_LENGTH;

            // Precompute scaled colors
            uint8_t scaledR = colors[0] * scale;
            uint8_t scaledG = colors[1] * scale;
            uint8_t scaledB = colors[2] * scale;
            uint8_t scaledW = colors[3] * scale;

            pixels.setPixelColor(tailIndex, pixels.Color(scaledR, scaledG, scaledB, scaledW));
        }

        // Update position and timing
        lastMoveMs = currentMs;
        nextPosition = (nextPosition + 1) % NUM_PIXELS;
    }
}

long clockLastMoveMs = -1;
long clockPulseStep = 0;
float clockPulseSteps = CLOCK_PULSE_MS / CLOCK_DELAY_MS;

void clock(int colors[4], long elapsedMs, int totalMins)
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
            float scaledW = colors[3] * scale;

            pixels.setPixelColor(i, pixels.Color(scaledR, scaledG, scaledB, scaledW));
        }

        // Pulse the lead pixel
        float pulseScale = abs(clockPulseStep - (CLOCK_PULSE_MS / 2.0)) / (CLOCK_PULSE_MS / 2.0);
        float pulsedR = colors[0] * pulseScale;
        float pulsedG = colors[1] * pulseScale;
        float pulsedB = colors[2] * pulseScale;
        float pulsedW = colors[3] * pulseScale;

        pixels.setPixelColor(lastPixelIndex - 1, pixels.Color(pulsedR, pulsedG, pulsedB, pulsedW));

        // Prepare next step
        clockPulseStep = clockPulseStep + CLOCK_DELAY_MS > CLOCK_PULSE_MS ? 0 : clockPulseStep + CLOCK_DELAY_MS;
        clockLastMoveMs = clockMs;
    }
}