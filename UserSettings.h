/*
 * Student: Bruno Luiz da Silva
 * ID: 150724708
 */
#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

#define BEAGLERT_FREQ   44100.00
#define FILTER_ORDER    2
#define FILTER_NUM      2

struct UserSettings{
    float frequency;
    bool butterworth;
    bool linkwitz;
    bool bassboost;
};

#endif