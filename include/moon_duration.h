/*
 * moon_duration.h
 * Calcul de la durée entre le lever et le coucher de la Lune pour une journée
 * locale.
 */

#ifndef MOON_DURATION_H
#define MOON_DURATION_H

typedef struct {
    int has_rise;
    int has_set;
    // Lune visible toute la journée (aucun lever ni coucher, au-dessus de
    // l'horizon) ou absente toute la journée (sous l'horizon).
    int all_day;
    int none;
    double rise_julian_day;
    double set_julian_day;
    double duration_hours;
} MoonDuration;

int moon_duration_compute(double day_start_jd, double day_end_jd,
                          double latitude, double longitude,
                          MoonDuration *out);

#endif /* MOON_DURATION_H */