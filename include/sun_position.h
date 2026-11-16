/*
 * sun_position.h
 * Position géocentrique du Soleil (éléments orbitaux de Schlyter) à partir d'un
 * Julian Day. Elle alimente les perturbations lunaires et l'orientation du
 * limbe brillant.
 */

#ifndef SUN_POSITION_H
#define SUN_POSITION_H

typedef struct {
    double right_ascension;    /* [0, 360) */
    double declination;        /* [-90, 90] */
    double ecliptic_longitude; /* [0, 360) */
    double mean_longitude;     /* [0, 360), pour les perturbations lunaires */
    double mean_anomaly;       /* [0, 360) */
} SunPosition;

void sun_position_at(double julian_day, SunPosition *out);

#endif /* SUN_POSITION_H */