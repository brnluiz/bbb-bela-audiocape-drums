#include <BeagleRT.h>
#include <Utilities.h>
#include "utils.h"
#include "settings.h"
#include "Filter.h"

extern Filter *filter;

void led(BeagleRTContext *context, int status) {
	if (status == GPIO_LOW) { //toggle the status
		digitalWriteFrame(context, 0, LED_PIN, status); //write the status to the LED
		status = GPIO_HIGH;
	}
	else {
		digitalWriteFrame(context, 0, LED_PIN, status); //write the status to the LED
		status = GPIO_LOW;
	}
}

float potentiometer(BeagleRTContext *context, int n) {
	int audioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	float input = analogReadFrame(context, n/audioFramesPerAnalogFrame, POT_PIN);
	return map(input, 0, .85, 0.05, 1);
}

float accelerometer(BeagleRTContext *context, int n, int direction, float minVal = .3, float maxVal = .55) {
	int audioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	int pin;

	if (direction == READ_X) {
		pin = ACCL_X_PIN;
	} else if (direction == READ_Y) {
		pin = ACCL_Y_PIN;
	} else if (direction == READ_Z) {
		pin = ACCL_Z_PIN;
	} else {
		return 0;
	}

	float input = analogReadFrame(context, n/audioFramesPerAnalogFrame, pin);
	float mapped = map(input, minVal, maxVal, -1, 1);
	
	if(mapped > 1) {
		mapped = 1;
	}
	else if(mapped < -1) {
		mapped = -1;
	}

	// Debug
	// if (direction == READ_X) {
	// 	rt_printf("x: %fg | value: %f\n", mapped, input);
	// } else if (direction == READ_Y) {
	// 	rt_printf("y: %fg | value: %f\n", mapped, input);
	// } else if (direction == READ_Z) {
	// 	rt_printf("z: %fg | value: %f\n", mapped, input);
	// }
	
	return mapped;
}

Orientation getOrientation(BeagleRTContext *context, int n) {
	const float threshold = .7;
	Orientation orientation;
	float x = accelerometer(context, n, READ_X);
	float y = accelerometer(context, n, READ_Y);
	float z = accelerometer(context, n, READ_Z);

	// Compare the axis based on the threshold and then decide the orientation
	if (x > threshold) {
		orientation = VERTICAL_RIGHT;
	} else if (x < -threshold) {
		orientation = VERTICAL_LEFT;
	} else if (y > threshold) {
		orientation = VERTICAL_BACK;
	} else if (y < -threshold) {
		orientation = VERTICAL_FRONT;
	} else if (z > threshold) {
		orientation = RESTING;
	} else if (z < -threshold) {
		orientation = REVERSE;
	} else {
		// orientation = getOrientation(context, n);
		orientation = RESTING;
	}

	return orientation;
}

bool getBoardTap(BeagleRTContext *context, int n) {
	int axis = ACCL_Z_PIN;
	const float threshold = 0.3;

	Orientation orientation = getOrientation(context, n);
	if (orientation ==  VERTICAL_RIGHT) {
		axis = ACCL_X_PIN;
	} else if (orientation == VERTICAL_LEFT) {
		axis = ACCL_X_PIN;
	} else if (orientation ==  VERTICAL_BACK) {
		axis = ACCL_Y_PIN;
	} else if (orientation == VERTICAL_FRONT) {
		axis = ACCL_Y_PIN;
	} else if (orientation == RESTING) {
		axis = ACCL_Z_PIN;
	} else if (orientation == REVERSE) {
		return false;
	} else {
		return false;
	}

	int audioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	float input = analogReadFrame(context, n/audioFramesPerAnalogFrame, axis);
	
	float filtered = filter->run(input);

	if (filtered > 0.2) {
		rt_printf("axis: %d | data: %f | filtered: %f\n", axis, input, filtered);
		return true;
	}

	return false;
}