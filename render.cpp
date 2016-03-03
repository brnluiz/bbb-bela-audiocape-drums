/*
 * assignment2_drums
 *
 * Second assignment for ECS732 RTDSP, to create a sequencer-based
 * drum machine which plays sampled drum sounds in loops.
 *
 * This code runs on BeagleBone Black with the Bela/BeagleRT environment.
 *
 * 2016 
 * Becky Stewart
 *
 * 2015
 * Andrew McPherson and Victor Zappi
 * Queen Mary University of London
 */


#include <Utilities.h>
#include <BeagleRT.h>
#include <rtdk.h>
#include <cmath>
#include "drums.h"
#include "settings.h"
#include "utils.h"

/* Variables which are given to you: */

/* Drum samples are pre-loaded in these buffers. Length of each
 * buffer is given in gDrumSampleBufferLengths.
 */
extern float *gDrumSampleBuffers[NUMBER_OF_DRUMS];
extern int gDrumSampleBufferLengths[NUMBER_OF_DRUMS];

int gIsPlaying = 0;			/* Whether we should play or not. Implement this in Step 4b. */

/* Read pointer into the current drum sample buffer.
 *
 * TODO (step 3): you will replace this with two arrays, one
 * holding each read pointer, the other saying which buffer
 * each read pointer corresponds to.
 */
int gReadPointer[SLOTS_SIZE] = {-1};

/* Patterns indicate which drum(s) should play on which beat.
 * Each element of gPatterns is an array, whose length is given
 * by gPatternLengths.
 */
extern int *gPatterns[NUMBER_OF_PATTERNS];
extern int gPatternLengths[NUMBER_OF_PATTERNS];

/* These variables indicate which pattern we're playing, and
 * where within the pattern we currently are. Used in Step 4c.
 */
int gCurrentPattern = 0;
int gCurrentIndexInPattern = 0;

/* This variable holds the interval between events in **milliseconds**
 * To use it (Step 4a), you will need to work out how many samples
 * it corresponds to.
 */
float gEventInterval = 1;
unsigned int gEventCounter = 0;

/* This variable indicates whether samples should be triggered or
 * not. It is used in Step 4b, and should be set in gpio.cpp.
 */
extern int gIsPlaying;

/* This indicates whether we should play the samples backwards.
 */
int gPlaysBackwards = 0;

/* For bonus step only: these variables help implement a fill
 * (temporary pattern) which is triggered by tapping the board.
 */
int gShouldPlayFill = 0;
int gPreviousPattern = 0;

/* TODO: Declare any further global variables you need here */
int gButtonPin[BUTTONS_SIZE]     = {BUTTON1_PIN, BUTTON2_PIN}; // Stores the buttons pins specifications
int gButtonPressed[BUTTONS_SIZE] = {-1}; // Stores the button pressed state
RenderStates gStates[DRUMS_SIZE] = {STATE_WAIT}; // States for render() function

int gDrumBufferForReadPointer[SLOTS_SIZE] = {-1};

float out[SLOTS_SIZE] = {0}; // Output array
int gAudioFramesPerAnalogFrame;
// setup() is called once before the audio rendering starts.
// Use it to perform any initialisation and allocation which is dependent
// on the period size or sample rate.
//
// userData holds an opaque pointer to a data structure that was passed
// in from the call to initAudio().
//
// Return true on success; returning false halts the program.

bool setup(BeagleRTContext *context, void *userData)
{
	/* Step 2: initialise GPIO pins */
	pinModeFrame(context, 0, BUTTON1_PIN, INPUT);
	pinModeFrame(context, 0, BUTTON2_PIN, INPUT);
	pinModeFrame(context, 0, LED_PIN, OUTPUT);

	for(int i = 0; i < SLOTS_SIZE; i++) {
		gDrumBufferForReadPointer[i] = -1;
	}
	for(int i = 0; i < SLOTS_SIZE; i++) {
		gReadPointer[i] = -1;
	}
	for(int i = 0; i < SLOTS_SIZE; i++) {
		gButtonPressed[i] = -1;
	}

	gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

	return true;
}

// render() is called regularly at the highest priority by the audio engine.
// Input and output are given from the audio hardware and the other
// ADCs and DACs (if available). If only audio is available, numMatrixFrames
// will be 0.
void render(BeagleRTContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		
		// Read button and toogle play state
		gButtonPressed[0] = !digitalReadFrame(context, n, gButtonPin[0]);
		if(gButtonPressed[0]) {
			gIsPlaying = !gIsPlaying;
		}

		// Read potentiometer and adjust the event interval accordingly
		if(!(n % gAudioFramesPerAnalogFrame)) {
			gEventInterval = potentiometer(context, n);
		}

		// Start the next event depending on the gEventInterval setting (gEventCounter have to be = to gEventInterval)
		if(gEventCounter >= gEventInterval * context->audioSampleRate) {
			if (gIsPlaying) {
				startNextEvent();
			}
			gEventCounter = 0;
		}
		gEventCounter++;
		
		// Loop through all gReadPointer slots and play the setted ones (when gReadPointer != -1)
		for(int slot = 0; slot < SLOTS_SIZE; slot++) {
			int drum = 0; // Will be used to store the drum that will be used

			switch(gStates[slot]) {
	            case STATE_WAIT:
	                // If there is an associated drum buffer AND the read pointer is set to 0 (start to play), go to STATE_PLAYING
	                if (gDrumBufferForReadPointer[slot] != -1 && gReadPointer[slot] >= 0) {
	                    gStates[slot] = STATE_PLAYING;
	                    led(context, GPIO_HIGH);
	                }
	                break;

	            case STATE_PLAYING:
	                // If the gReadPointer reached the end (drum length), go to the STATE_END
	                if (gReadPointer[slot] >= gDrumSampleBufferLengths[drum]) {
	                    gStates[slot] = STATE_END;
	                }

	            	// Get the drum (8 available types)
	                drum = gDrumBufferForReadPointer[slot];

	                // Save the drum sample to the output variable
	                out[slot] = gDrumSampleBuffers[drum][ gReadPointer[slot] ];

	                // Increment the read pointer of this slot so the program can read the next samples
	                gReadPointer[slot]++;

	                break;

	            case STATE_END:
	            	// Reset all needed variables to the initial values
	            	gReadPointer[slot] = -1;
	            	gDrumBufferForReadPointer[slot] = -1;
	            	gStates[slot] = STATE_WAIT;
	            	led(context, GPIO_LOW);
	        }
		}

		// Mix all slots and output it to each channel
		for(unsigned int channel = 0; channel < context->audioChannels; channel++) {
			for(int slot = 0; slot < SLOTS_SIZE; slot++) {
				context->audioOut[n * context->audioChannels + channel] += out[slot];
			}
			context->audioOut[n * context->audioChannels + channel] = context->audioOut[n * context->audioChannels + channel]/SLOTS_SIZE;
        }
	}
}

/* Start playing a particular drum sound given by drumIndex.
 */
void startPlayingDrum(int drumIndex) {
	// Get the first empty read pointer
	for(int i = 0; i < SLOTS_SIZE; i++) {
		if(gReadPointer[i] == -1 && gDrumBufferForReadPointer[i] == -1) {
			// Which drum will be played
			gDrumBufferForReadPointer[i] = drumIndex;

			// Reset gReadPointer
			gReadPointer[i] = 0;

			// rt_printf("Chosen slot: %d\n", i);
			return;
		}
	}

	// No read pointer available
	// rt_printf("No read pointer available\n");
}

/* Start playing the next event in the pattern */
void startNextEvent() {
	startPlayingDrum(0);
}

/* Returns whether the given event contains the given drum sound */
int eventContainsDrum(int event, int drum) {
	if(event & (1 << drum))
		return 1;
	return 0;
}

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
	float pot = analogReadFrame(context, n/gAudioFramesPerAnalogFrame, POT_PIN);
	return map(pot, 0, .85, 0.05, 1);
}

// cleanup_render() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in initialise_render().

void cleanup(BeagleRTContext *context, void *userData)
{

}
