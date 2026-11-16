/*
 * moon_position.h
 * Position géocentrique de la Lune et altitude apparente pour un observateur.
 */

#ifndef MOON_POSITION_H
#define MOON_POSITION_H

typedef struct {
    double right_ascension;
    double declination;
    double ecliptic_longitude;
    double ecliptic_latitude;
    double distance_km;
} MoonPosition;

void moon_position_at(double julian_day, MoonPosition *out);

double moon_altitude(double julian_day, double latitude_deg,
                     double longitude_deg);

#endif /* MOON_POSITION_H */