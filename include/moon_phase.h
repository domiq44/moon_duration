/*
 * moon_phase.h
 * Calcul de la phase lunaire et de l'illumination à partir d'un Julian Day.
 */

#ifndef MOON_PHASE_H
#define MOON_PHASE_H

typedef enum {
    MOON_PHASE_INVALID = -1,
    MOON_NEW,
    MOON_WAXING_CRESCENT,
    MOON_FIRST_QUARTER,
    MOON_WAXING_GIBBOUS,
    MOON_FULL,
    MOON_WANING_GIBBOUS,
    MOON_LAST_QUARTER,
    MOON_WANING_CRESCENT
} MoonPhase;

MoonPhase moon_phase_from_julian_day(double julian_day);

double moon_illumination(double julian_day);

#endif /* MOON_PHASE_H */