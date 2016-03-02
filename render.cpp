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

/* Triggers from buttons (step 2 etc.). Read these here and
 * do something if they are nonzero (resetting them when done). */
int gTriggerButton1;
int gTriggerButton2;

/* This variable holds the interval between events in **milliseconds**
 * To use it (Step 4a), you will need to work out how many samples
 * it corresponds to.
 */
int gEventIntervalMilliseconds = 250;

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
int gButtonPin[BUTTONS_SIZE]     = {BUTTON1_PIN, BUTTON2_PIN};
int gButtonPressed[BUTTONS_SIZE] = {-1};
RenderStates gStates[DRUMS_SIZE] = {STATE_WAIT};

int gDrumBufferForReadPointer[SLOTS_SIZE] = {-1};

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

	for(int i = 0; i < SLOTS_SIZE; i++) {
		gDrumBufferForReadPointer[i] = -1;
	}
	for(int i = 0; i < SLOTS_SIZE; i++) {
		gReadPointer[i] = -1;
	}
	for(int i = 0; i < SLOTS_SIZE; i++) {
		gButtonPressed[i] = -1;
	}

	return true;
}

// render() is called regularly at the highest priority by the audio engine.
// Input and output are given from the audio hardware and the other
// ADCs and DACs (if available). If only audio is available, numMatrixFrames
// will be 0.
int debounce[2] = {0};
int debounceCheck[2] = {0};
void render(BeagleRTContext *context, void *userData)
{
	/* TODO: your audio processing code goes here! */

	for(unsigned int n = 0; n < context->audioFrames; n++) {
		float out[SLOTS_SIZE] = {0};
		int drum = 0;

		// Check which button is pressed and play the associated drum
		for(int i = 0; i < BUTTONS_SIZE; i++) {
			gButtonPressed[i] = !digitalReadFrame(context, 0, gButtonPin[i]);
			
			if(gButtonPressed[i] && debounceCheck[i] == 0) {
	            gButtonPressed[i] = 0;
	            rt_printf("Button %d pressed", i);
	            debounceCheck[i] = 1;
	            startPlayingDrum(i);
	        }
	        debounce[i]++;

	        if(debounce[i] >= 1 * context->audioSampleRate) {
	        	debounceCheck[i] = 0;
	        }
		}

		// Loop through all gReadPointer slots and play the setted ones (when gReadPointer != -1)
		for(int slot = 0; slot < SLOTS_SIZE; slot++) {

			switch(gStates[slot]) {
	            case STATE_WAIT:
	                // If there is an associated drum buffer AND the read pointer is set to 0 (start to play), go to STATE_PLAYING
	                if (gDrumBufferForReadPointer[slot] != -1 && gReadPointer[slot] >= 0) {
	                    gStates[slot] = STATE_PLAYING;
	                }
	                break;

	            case STATE_PLAYING:
	                // If the gReadPointer reached the end (drum length), go to the STATE_END
	                if (gReadPointer[slot] >= gDrumSampleBufferLengths[drum]) {
	                    gStates[slot] = STATE_END;
	                }

	            	// Get the setted drum (16 types)
	                drum = gDrumBufferForReadPointer[slot];

	                // Output the drum
	                out[slot] = gDrumSampleBuffers[drum][ gReadPointer[slot] ];

	                // Increment the read pointer of this slot
	                gReadPointer[slot]++;

	                break;

	            case STATE_END:
	            	// Reset all pointers to initial values
					// rt_printf("readpointer value: %d | slot: %d\n", gReadPointer[slot], slot);
	            	gReadPointer[slot] = -1;
	            	gDrumBufferForReadPointer[slot] = -1;
	            	gStates[slot] = STATE_WAIT;
	        }
		}

		for(unsigned int channel = 0; channel < context->audioChannels; channel++) {
			for(int slot = 0; slot < SLOTS_SIZE; slot++) {
				context->audioOut[n * context->audioChannels + channel] += out[slot];
			}
			context->audioOut[n * context->audioChannels + channel] = context->audioOut[n * context->audioChannels + channel]/SLOTS_SIZE;
        }
	}
	/* Step 3: use multiple read pointers to play multiple drums */

	/* Step 4: count samples and decide when to trigger the next event */
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

			rt_printf("Chosen slot: %d\n", i);
			return;
		}
	}

	// No read pointer available
	// rt_printf("No read pointer available\n");
}

/* Start playing the next event in the pattern */
void startNextEvent() {
	/* TODO in Step 4 */
}

/* Returns whether the given event contains the given drum sound */
int eventContainsDrum(int event, int drum) {
	if(event & (1 << drum))
		return 1;
	return 0;
}

// cleanup_render() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in initialise_render().

void cleanup(BeagleRTContext *context, void *userData)
{

}
