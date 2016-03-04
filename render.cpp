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
#include "Filter.h"

/* Variables which are given to you: */

/* Drum samples are pre-loaded in these buffers. Length of each
 * buffer is given in gDrumSampleBufferLengths.
 */
extern float *gDrumSampleBuffers[NUMBER_OF_DRUMS];
extern int gDrumSampleBufferLengths[NUMBER_OF_DRUMS];

Filter *filter;

int gIsPlaying = 1;			/* Whether we should play or not. Implement this in Step 4b. */

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
bool gShouldPlayFill = false;
int gPreviousPattern = 0;
int gPreviousIndex = 0;

/* TODO: Declare any further global variables you need here */
int gButtonPin[BUTTONS_SIZE]     = {BUTTON1_PIN, BUTTON2_PIN}; // Stores the buttons pins specifications
int gButtonPressed[BUTTONS_SIZE] = {-1}; // Stores the button pressed state
RenderStates gStates[DRUMS_SIZE] = {STATE_WAIT}; // States for render() function

int gDrumBufferForReadPointer[SLOTS_SIZE] = {-1};

float out[SLOTS_SIZE] = {0}; // Output array
int gAudioFramesPerAnalogFrame;
int gOrientation = 0;
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
		gReadPointer[i] = -1;
	}
	for(int i = 0; i < BUTTONS_SIZE; i++) {
		gButtonPressed[i] = -1;
	}

	gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

	filter = new Filter(HIGH_PASS, 50);

	return true;
}

// render() is called regularly at the highest priority by the audio engine.
// Input and output are given from the audio hardware and the other
// ADCs and DACs (if available). If only audio is available, numMatrixFrames
// will be 0.
int counter = 0;
int initFilter = 0;
void render(BeagleRTContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		
		// Read button and toogle play state
		if(n == 0) {
			gButtonPressed[0] = !digitalReadFrame(context, n, gButtonPin[0]);
			if(gButtonPressed[0]) {
				gIsPlaying = !gIsPlaying;
			}
		}

		// Read potentiometer and adjust the event interval accordingly
		if(!(n % gAudioFramesPerAnalogFrame)) {
			gEventInterval = potentiometer(context, n);
		}

		// Check the board for taps... If a tap happened, then it will not check until next render() call
		if(!gShouldPlayFill) {
			if (getBoardTap(context, n) && initFilter == FILTER_INIT_SAMPLES) {
				rt_printf("Fill!\n");
				gShouldPlayFill = true;
				gPreviousPattern = gCurrentPattern;
				gPreviousIndex   = gCurrentIndexInPattern;
				
				gCurrentPattern = FILL_PATTERN;
				gCurrentIndexInPattern = 0;
			} else if (initFilter < FILTER_INIT_SAMPLES) {
				initFilter++;
			}
		}

		// Read the accelerometer for the orientation
		if(!(n % 100) && !gShouldPlayFill) {
			int orientation = (int)getOrientation(context, n);

			if (orientation != REVERSE) {
				gCurrentPattern = orientation;
				gPlaysBackwards = false;
			} else {
				gPlaysBackwards = true;
			}
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
			int pos  = 0;

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

	                // Decide to play it normally or backwards (depends on the orientation)
	                pos = gReadPointer[slot];
	                if (gPlaysBackwards) {
	                	pos = gDrumSampleBufferLengths[drum] - pos;
	                }
	                
	                // Save the drum sample to the output variable
	                out[slot] = gDrumSampleBuffers[drum][pos];

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
	// Reset the pattern index (start from the beginning again)
	if(gCurrentIndexInPattern >= gPatternLengths[gCurrentPattern]) {
		if (gShouldPlayFill) {
			// rt_printf("shouldFill end | pattern: %d | gCurrentIndex: %d | Pattern Length: %d\n", gCurrentPattern, gCurrentIndexInPattern, gPatternLengths[gCurrentPattern]);
			gCurrentPattern = gPreviousPattern;
			gCurrentIndexInPattern = gPreviousIndex;
			// rt_printf("shouldFill reset | pattern: %d | gCurrentIndex: %d | Pattern Length: %d\n", gCurrentPattern, gCurrentIndexInPattern, gPatternLengths[gCurrentPattern]);
			gShouldPlayFill = false;
			initFilter = 0;
		} else {
			gCurrentIndexInPattern = 0;
		}
	}

	// When the board orietation changes, check if the current index would not run out of boundaries
	gCurrentIndexInPattern = gCurrentIndexInPattern % gPatternLengths[gCurrentPattern];

	int event = gPatterns[gCurrentPattern][gCurrentIndexInPattern];

	// Check if the gPattern actual item have or not a valid drum
	for(int drum = 0; drum < DRUMS_SIZE; drum++) {
		// Play the configured pattern
		if(eventContainsDrum(event, drum)) {
			startPlayingDrum(drum);
		}
	}

	gCurrentIndexInPattern++;
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
	delete filter;
}
