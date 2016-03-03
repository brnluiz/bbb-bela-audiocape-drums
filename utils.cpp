#include <BeagleRT.h>
#include <Utilities.h>
#include "utils.h"
#include "settings.h"

float acclMaxValue = 0, acclMinValue = 1;

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

// void setupAcelerometer(BeagleRTContext *context, int n) {
//     int audioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

//     for (int i = 0; i < 100; i++) {
//         for (int j = 0; j < 3; j++) {
//             float input = analogReadFrame(context, n/audioFramesPerAnalogFrame, pin);
//             if (input < acclMinValue) {
//                 acclMinValue = input;
//             }
//             if (input > acclMaxValue) {
//                 acclMaxValue = input;
//             }
//         }
//     }
// }

float accelerometer(BeagleRTContext *context, int n, int direction) {
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
	float mapped = map(input, .3, .55, -1, 1);
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