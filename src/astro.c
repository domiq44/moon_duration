/*
 * astro.c
 * Conversions et constantes astronomiques partagées par les modules
 * d'éphémérides (Lune, Soleil) et d'orientation.
 */

#include "astro.h"

#include <math.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ASTRO_DEG2RAD (M_PI / 180.0)
#define ASTRO_RAD2DEG (180.0 / M_PI)
#define ASTRO_JULIAN_EPOCH_2000 2451545.0

double astro_norm_360(double angle) {
  angle = fmod(angle, 360.0);

  if (angle < 0.0) {
    angle += 360.0;
  }

  return angle;
}

double astro_norm_180(double angle) {
  angle = fmod(angle, 360.0);

  if (angle >= 180.0) {
    angle -= 360.0;
  } else if (angle < -180.0) {
    angle += 360.0;
  }

  return angle;
}

double astro_sind(double degrees) { return sin(degrees * ASTRO_DEG2RAD); }

double astro_cosd(double degrees) { return cos(degrees * ASTRO_DEG2RAD); }

double astro_atan2d(double y, double x) { return atan2(y, x) * ASTRO_RAD2DEG; }

double astro_asind(double value) {
  if (value > 1.0) {
    value = 1.0;
  } else if (value < -1.0) {
    value = -1.0;
  }

  return asin(value) * ASTRO_RAD2DEG;
}

// Obliquité de l'écliptique en degrés (Meeus 22.2).
static double obliquity_degrees(double julian_day) {
  const double t = (julian_day - ASTRO_JULIAN_EPOCH_2000) / 36525.0;

  return 23.4392911 - 0.0130042 * t - 1.64e-7 * t * t + 5.04e-7 * t * t * t;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
void astro_ecliptic_to_equatorial(double ecliptic_longitude_deg,
                                  double ecliptic_latitude_deg,
                                  double julian_day,
                                  double *right_ascension_deg,
                                  double *declination_deg) {
  const double epsilon = obliquity_degrees(julian_day);
  const double x =
      astro_cosd(ecliptic_longitude_deg) * astro_cosd(ecliptic_latitude_deg);
  const double y =
      astro_sind(ecliptic_longitude_deg) * astro_cosd(ecliptic_latitude_deg);
  const double z = astro_sind(ecliptic_latitude_deg);
  const double y_equatorial = y * astro_cosd(epsilon) - z * astro_sind(epsilon);
  const double z_equatorial = y * astro_sind(epsilon) + z * astro_cosd(epsilon);

  if (right_ascension_deg != NULL) {
    *right_ascension_deg = astro_norm_360(astro_atan2d(y_equatorial, x));
  }

  if (declination_deg != NULL) {
    *declination_deg = astro_asind(z_equatorial);
  }
}
// NOLINTEND(bugprone-easily-swappable-parameters)

double astro_sidereal_mean_degrees(double julian_day) {
  const double t = (julian_day - ASTRO_JULIAN_EPOCH_2000) / 36525.0;
  const double days = julian_day - ASTRO_JULIAN_EPOCH_2000;

  return astro_norm_360(280.46061837 + 360.98564736629 * days +
                        0.000387933 * t * t - t * t * t / 38710000.0);
}