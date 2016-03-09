/*
 * Student: Bruno Luiz da Silva
 * ID: 150724708
 */
#ifndef UTILS_H
#define UTILS_H

#include "settings.h"

void led(BeagleRTContext *context, int status);
float potentiometer(BeagleRTContext *context, int n);
float accelerometer(BeagleRTContext *context, int n, int direction, float minVal, float maxVal);

Orientation getOrientation(BeagleRTContext *context, int n);
bool getBoardTap(BeagleRTContext *context, int n);

#endif