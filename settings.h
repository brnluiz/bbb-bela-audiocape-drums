#ifndef SETTINGS_H
#define SETTINGS_H

#include <BeagleRT.h>

#define BEAGLERT_FREQ   44100.00

// GPIO Configs
#define BUTTONS_SIZE 2
#define BUTTON1_PIN P8_07
#define BUTTON2_PIN P8_08
#define LED_PIN P8_09

// Analog Input Configs
#define POT_PIN 0
#define ACCL_X_PIN 1
#define ACCL_Y_PIN 2
#define ACCL_Z_PIN 3

// Sizes definitions
#define DRUMS_SIZE 8
#define SLOTS_SIZE 16

#define FILTER_INIT_SAMPLES 100

// Render states
typedef enum {
    STATE_WAIT,
    STATE_PLAYING,
    STATE_END
} RenderStates;

enum {
    READ_X,
    READ_Y,
    READ_Z
};

typedef enum {
    RESTING,
    VERTICAL_LEFT,
    VERTICAL_RIGHT,
    VERTICAL_FRONT,
    VERTICAL_BACK,
    REVERSE
} Orientation;

#endif