/*
 * Student: Bruno Luiz da Silva
 * ID: 150724708
 */

#ifndef FILTER
#define FILTER

#define MAX_FILTER_ORDER 8

typedef enum {
    LOW_PASS,
    HIGH_PASS,
} FilterType;

struct FilterParameters {
    int order;

    float a[MAX_FILTER_ORDER];
    float b[MAX_FILTER_ORDER];

    float x[MAX_FILTER_ORDER];
    float y[MAX_FILTER_ORDER];
};

class Filter {
public:
    Filter(FilterType type, float freq);
    float run(float sample);
private:
    FilterParameters filter_;
};

#endif