#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
// This lib is fine for now but there are better ones
#include <Button.h>

#define NEOPIXEL_PIN 12
#define NUM_PIXELS 24

Button mainButton(7);

Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
    mainButton.begin();
    pixels.begin();

    Serial.begin(9600);
}

void loop()
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        pixels.setPixelColor(i, pixels.Color(0, 150, 0));
        pixels.show();
    }

    if (mainButton.pressed())
        Serial.println("Button pressed");

    if (mainButton.released())
        Serial.println("Button released");
}