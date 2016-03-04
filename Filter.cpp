#include <BeagleRT.h>
#include "Filter.h"
#include "settings.h"
#include <cmath>

Filter::Filter(FilterType type, float freq) {
    const float c = 2 * BEAGLERT_FREQ;
    const float d = c * 1.4142;
    float w = 2 * M_PI * freq;

    FilterParameters lowPass;
    FilterParameters highPass;

    // Low pass filter calcs
    lowPass.order  = 2;

    lowPass.a[0] = pow(c,2) + d*w + pow(w,2);
    lowPass.a[1] = (2*pow(w,2) - 2*pow(c,2));
    lowPass.a[2] = (pow(c,2) - d*w + pow(w,2));        

    lowPass.b[0] = (pow(w,2));
    lowPass.b[1] = (2*pow(w,2));
    lowPass.b[2] = (pow(w,2));

    // High pass filter calcs
    highPass.order = 2;

    highPass.a[0] = lowPass.a[0];
    highPass.a[1] = lowPass.a[1];
    highPass.a[2] = lowPass.a[2];
    
    highPass.b[0] = (pow(c,2));
    highPass.b[1] = -(2*pow(c,2));
    highPass.b[2] = (pow(c,2));

    // Reset some parameters and divide the others by the common factor a[0]
    for(int i = 0; i < MAX_FILTER_ORDER; i++) {
        lowPass.x[i]  = 0;
        lowPass.y[i]  = 0;
        highPass.x[i] = 0;
        highPass.y[i] = 0;
        
        lowPass.b[i]  = lowPass.b[i]/lowPass.a[0];
        highPass.b[i] = highPass.b[i]/highPass.a[0];
        
        if(i != 0) {
            lowPass.a[i]  = lowPass.a[i]/lowPass.a[0];
            highPass.a[i] = highPass.a[i]/highPass.a[0];
        }
    }

    // Divide the a[0]
    lowPass.a[0]  = 1;
    highPass.a[0] = 1;

    // Define the filter_ variable
    if (type == LOW_PASS) { 
        filter_ = lowPass;
    } else if (type == HIGH_PASS) {
        filter_ = highPass;
    }
}

float Filter::run(float sample) {
    float out = 0;

    // Create the filter_ output
    for(int i = 1; i <= filter_.order; i++) {
        out += filter_.b[i] * filter_.x[i-1];
        out -= filter_.a[i] * filter_.y[i-1];
    }
    out += filter_.b[0] * sample;

    // Save data for later
    for (int i = 1; i < filter_.order; i++) {
        filter_.x[i] = filter_.x[i-1];
        filter_.y[i] = filter_.y[i-1];
    }
    filter_.x[0] = sample;
    filter_.y[0] = out;

    return out;
}