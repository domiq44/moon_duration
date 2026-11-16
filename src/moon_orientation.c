/*
 * moon_orientation.c
 * Orientation du limbe brillant de la Lune.
 *
 * L'angle géocentrique χ (Meeus 48.4) est l'angle de position du milieu du
 * limbe éclairé, compté depuis le nord céleste vers l'est. L'orientation
 * apparente pour un observateur s'obtient en retranchant l'angle parallactique
 * q, c'est-à-dire la position du zénith par rapport au nord céleste :
 *
 *     χ' = χ - q
 *
 * Le Soleil et la Lune étant quasi coplanaires avec l'écliptique, χ vaut
 * environ 90° à la nouvelle lune et 270° à la pleine lune : le limbe est alors
 * indéterminé, ce que traduit le masquage de l'illumination.
 */

#include "moon_orientation.h"

#include <math.h>

#include "astro.h"
#include "moon_phase.h"
#include "moon_position.h"
#include "sun_position.h"

/* En dessous de 2 % d'illumination, la Lune est trop fine pour qu'un limbe
   soit identifiable ; au-dessus de 98 %, le disque est éclairé en entier. */
#define ORIENTATION_MIN_ILLUMINATION 0.02
#define ORIENTATION_MAX_ILLUMINATION 0.98

// Indique si le limbe est identifiable à cet instant.
static int limb_is_visible(double julian_day) {
  const double illumination = moon_illumination(julian_day);

  return isfinite(illumination) &&
         illumination >= ORIENTATION_MIN_ILLUMINATION &&
         illumination <= ORIENTATION_MAX_ILLUMINATION;
}

double moon_bright_limb_angle_geocentric(double julian_day) {
  if (!isfinite(julian_day) || !limb_is_visible(julian_day)) {
    return NAN;
  }

  MoonPosition moon;
  SunPosition sun;
  moon_position_at(julian_day, &moon);
  sun_position_at(julian_day, &sun);

  const double delta_ra =
      astro_norm_180(sun.right_ascension - moon.right_ascension);
  const double y = astro_cosd(sun.declination) * astro_sind(delta_ra);
  const double x = astro_sind(sun.declination) * astro_cosd(moon.declination) -
                   astro_cosd(sun.declination) * astro_sind(moon.declination) *
                       astro_cosd(delta_ra);

  return astro_norm_360(astro_atan2d(y, x));
}

double moon_parallactic_angle(double julian_day, double latitude_deg,
                              double longitude_deg) {
  if (!isfinite(julian_day) || !isfinite(latitude_deg) ||
      !isfinite(longitude_deg)) {
    return NAN;
  }

  MoonPosition moon;
  moon_position_at(julian_day, &moon);

  const double local_sidereal =
      astro_norm_360(astro_sidereal_mean_degrees(julian_day) + longitude_deg);
  const double hour_angle =
      astro_norm_180(local_sidereal - moon.right_ascension);

  // Forme multipliée par cos(latitude) : elle reste définie aux pôles, où la
  // tangente de la latitude diverge.
  const double y = astro_cosd(latitude_deg) * astro_sind(hour_angle);
  const double x = astro_sind(latitude_deg) * astro_cosd(moon.declination) -
                   astro_cosd(latitude_deg) * astro_sind(moon.declination) *
                       astro_cosd(hour_angle);

  return astro_atan2d(y, x);
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
double moon_bright_limb_angle(double julian_day, double latitude_deg,
                              double longitude_deg) {
  if (!isfinite(julian_day) || !isfinite(latitude_deg) ||
      !isfinite(longitude_deg)) {
    return NAN;
  }

  const double geocentric = moon_bright_limb_angle_geocentric(julian_day);

  if (!isfinite(geocentric)) {
    return NAN;
  }

  const double parallactic =
      moon_parallactic_angle(julian_day, latitude_deg, longitude_deg);

  return astro_norm_360(geocentric - parallactic);
}
// NOLINTEND(bugprone-easily-swappable-parameters)