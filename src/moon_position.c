/*
 * moon_position.c
 * Position géocentrique de la Lune (éléments orbitaux + perturbations
 * principales) et altitude apparente pour un observateur.
 */

#include "moon_position.h"

#include <math.h>
#include <stddef.h>

#include "astro.h"
#include "sun_position.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAD2DEG (180.0 / M_PI)
#define EARTH_RADIUS_KM 6378.14
#define SCHLYTER_EPOCH 2451543.5

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
// Calcule les coordonnées écliptiques et la distance lunaire en rayons
// terrestres, sans perturbations.
static void moon_ecliptic(double days, double *longitude, double *latitude,
                          double *distance) {
  const double node = astro_norm_360(125.1228 - 0.0529538083 * days);
  const double inclination = 5.1454;
  const double argument = astro_norm_360(318.0634 + 0.1643573223 * days);
  const double semi_major = 60.2666;
  const double eccentricity = 0.054900;
  const double mean_anomaly = astro_norm_360(115.3654 + 13.0649929509 * days);
  const double eccentric = eccentricity * RAD2DEG;
  const double e_anomaly =
      mean_anomaly + eccentric * astro_sind(mean_anomaly) *
                         (1.0 + eccentricity * astro_cosd(mean_anomaly));
  const double xv = semi_major * (astro_cosd(e_anomaly) - eccentricity);
  const double yv = semi_major * sqrt(1.0 - eccentricity * eccentricity) *
                    astro_sind(e_anomaly);
  const double true_anomaly = astro_atan2d(yv, xv);
  const double radius = sqrt(xv * xv + yv * yv);
  const double u = true_anomaly + argument;
  const double xh =
      radius * (astro_cosd(node) * astro_cosd(u) -
                astro_sind(node) * astro_sind(u) * astro_cosd(inclination));
  const double yh =
      radius * (astro_sind(node) * astro_cosd(u) +
                astro_cosd(node) * astro_sind(u) * astro_cosd(inclination));
  const double zh = radius * (astro_sind(u) * astro_sind(inclination));

  *longitude = astro_norm_360(astro_atan2d(yh, xh));
  *latitude = astro_atan2d(zh, sqrt(xh * xh + yh * yh));
  *distance = radius;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
// Applique les perturbations périodiques principales à la position lunaire.
static void apply_perturbations(double days, const SunPosition *sun,
                                double *longitude, double *latitude,
                                double *distance) {
  const double node = astro_norm_360(125.1228 - 0.0529538083 * days);
  const double argument = astro_norm_360(318.0634 + 0.1643573223 * days);
  const double mean_anomaly = astro_norm_360(115.3654 + 13.0649929509 * days);
  const double mean_longitude = astro_norm_360(mean_anomaly + argument + node);
  const double elongation =
      astro_norm_360(mean_longitude - sun->mean_longitude);
  const double argument_latitude = astro_norm_360(mean_longitude - node);
  const double m = mean_anomaly;
  const double ms = sun->mean_anomaly;
  const double d = elongation;
  const double f = argument_latitude;

  *longitude +=
      -1.274 * astro_sind(m - 2.0 * d) + 0.658 * astro_sind(2.0 * d) -
      0.186 * astro_sind(ms) - 0.059 * astro_sind(2.0 * m - 2.0 * d) -
      0.057 * astro_sind(m - 2.0 * d + ms) + 0.053 * astro_sind(m + 2.0 * d) +
      0.046 * astro_sind(2.0 * d - ms) + 0.041 * astro_sind(m - ms) -
      0.035 * astro_sind(d) - 0.031 * astro_sind(m + ms) -
      0.015 * astro_sind(2.0 * f - 2.0 * d) + 0.011 * astro_sind(m - 4.0 * d);
  *latitude +=
      -0.173 * astro_sind(f - 2.0 * d) - 0.055 * astro_sind(m - f - 2.0 * d) -
      0.046 * astro_sind(m + f - 2.0 * d) + 0.033 * astro_sind(f + 2.0 * d) +
      0.017 * astro_sind(2.0 * m + f);
  *distance += -0.58 * astro_cosd(m - 2.0 * d) - 0.46 * astro_cosd(2.0 * d);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

void moon_position_at(double julian_day, MoonPosition *out) {
  if (out == NULL) {
    return;
  }

  SunPosition sun;
  sun_position_at(julian_day, &sun);

  double longitude;
  double latitude;
  double distance;
  const double days = julian_day - SCHLYTER_EPOCH;
  moon_ecliptic(days, &longitude, &latitude, &distance);
  apply_perturbations(days, &sun, &longitude, &latitude, &distance);

  double right_ascension;
  double declination;
  astro_ecliptic_to_equatorial(longitude, latitude, julian_day,
                               &right_ascension, &declination);

  out->ecliptic_longitude = astro_norm_360(longitude);
  out->ecliptic_latitude = latitude;
  out->right_ascension = right_ascension;
  out->declination = declination;
  out->distance_km = distance * EARTH_RADIUS_KM;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
double moon_altitude(double julian_day, double latitude_deg,
                     double longitude_deg) {
  MoonPosition position;
  double local_sidereal;
  double hour_angle;
  double sin_altitude;

  moon_position_at(julian_day, &position);

  local_sidereal =
      astro_norm_360(astro_sidereal_mean_degrees(julian_day) + longitude_deg);
  hour_angle = astro_norm_360(local_sidereal - position.right_ascension);
  sin_altitude = astro_sind(latitude_deg) * astro_sind(position.declination) +
                 astro_cosd(latitude_deg) * astro_cosd(position.declination) *
                     astro_cosd(hour_angle);

  return astro_asind(sin_altitude);
}
// NOLINTEND(bugprone-easily-swappable-parameters)