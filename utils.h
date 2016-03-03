#ifndef UTILS_H
#define UTILS_H

#include "settings.h"

void led(BeagleRTContext *context, int status);
float potentiometer(BeagleRTContext *context, int n);
float accelerometer(BeagleRTContext *context, int n, int direction);

Orientation getOrientation(BeagleRTContext *context, int n);

#endif