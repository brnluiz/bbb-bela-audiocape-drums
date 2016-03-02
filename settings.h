#ifndef SETTINGS_H
#define SETTINGS_H

#include <BeagleRT.h>

#define BUTTONS_SIZE 2
#define BUTTON1_PIN P8_07
#define BUTTON2_PIN P8_08

#define DRUMS_SIZE 8
#define SLOTS_SIZE 16

typedef enum {
    STATE_WAIT,
    STATE_PLAYING,
    STATE_END
} RenderStates;

#endif