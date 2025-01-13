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
u_long lastButtonPress;

// Function definitions
void initializePixels();
void updatePomodoroCycle(u_long ms);
void updateLedRing(u_long ms);
void handleButtonPress(u_long ms);
void updateAnimation(u_long ms);
void chase(int colors[4]);
void clock(int colors[4], long elapsedMs, int totalMins);

void setup()
{
    mainButton.begin();
    pixels.begin();

    Serial.begin(115200);

    delay(100);

    initializePixels();
}

u_long lastLoop = 0;

void loop()
{
    u_long currentMs = millis();

    if (mainButton.pressed())
    {
        lastButtonPress = currentMs;
    }

    if (mainButton.released())
    {
        Serial.println("Button released");
        handleButtonPress(currentMs);
        lastButtonPress = -1;
    }

    if (currentMs - lastLoop > 10)
    {
        if (pomodoroMode != INACTIVE)
        {
            updatePomodoroCycle(currentMs);
            updateLedRing(currentMs);
        }

        updateAnimation(currentMs);

        pixels.show();

        lastLoop = currentMs;
    }
}

void handleButtonPress(u_long ms)
{
    if (pomodoroMode == INACTIVE)
    {
        pomodoroMode = WORK;
        pomodoroStartMs = ms;
    }
    else
    {
        // This will force `updatePomodoroCycle` to the next cycle
        pomodoroStartMs = -999999999;
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

    if (pomodoroMode == INACTIVE)
    {
        int colors[4] = {0, 0, 0, 100};

        chase(colors);
    }
    else if (pomodoroMode == WORK)
    {
        int colors[4] = {100, 0, 20, 0};

        clock(colors, elapsedMs, POMODORO_WORK_MINS);
    }
    else if (pomodoroMode == REST)
    {
        int colors[4] = {0, 20, 100, 0};

        clock(colors, elapsedMs, POMODORO_REST_MINS);
    }
    else if (pomodoroMode == BREAK)
    {
        int colors[4] = {0, 100, 20, 0};

        clock(colors, elapsedMs, POMODORO_BREAK_MINS);
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