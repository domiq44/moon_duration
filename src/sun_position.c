/*
 * sun_position.c
 * Position géocentrique apparente du Soleil (sans nutation ni aberration)
 * calculée à partir des éléments orbitaux de Schlyter.
 */

#include "sun_position.h"

#include <math.h>
#include <stddef.h>

#include "astro.h"

/* Époque de référence des éléments orbitaux (J2000 moins 1,5 jour). */
#define SUN_SCHLYTER_EPOCH 2451543.5

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAD2DEG (180.0 / M_PI)

void sun_position_at(double julian_day, SunPosition *out) {
  if (out == NULL) {
    return;
  }

  const double days = julian_day - SUN_SCHLYTER_EPOCH;
  const double perihelion = 282.9404 + 4.70935e-5 * days;
  const double eccentricity = 0.016709 - 1.151e-9 * days;
  const double mean_anomaly = astro_norm_360(356.0470 + 0.9856002585 * days);
  // Équation du centre : écart entre anomalie moyenne et anomalie vraie.
  const double eccentric_anomaly =
      mean_anomaly + eccentricity * RAD2DEG * astro_sind(mean_anomaly) *
                         (1.0 + eccentricity * astro_cosd(mean_anomaly));
  const double true_anomaly = astro_atan2d(
      sqrt(1.0 - eccentricity * eccentricity) * astro_sind(eccentric_anomaly),
      astro_cosd(eccentric_anomaly) - eccentricity);
  const double longitude = astro_norm_360(true_anomaly + perihelion);

  out->ecliptic_longitude = longitude;
  out->mean_longitude = astro_norm_360(mean_anomaly + perihelion);
  out->mean_anomaly = mean_anomaly;
  astro_ecliptic_to_equatorial(longitude, 0.0, julian_day,
                               &out->right_ascension, &out->declination);
}