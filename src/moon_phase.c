/*
 * moon_phase.c
 * Phase et illumination lunaire.
 *
 * La phase et la fraction éclairée sont dérivées de l'élongation entre la Lune
 * et le Soleil, obtenue à partir des positions écliptiques. La période
 * synodique moyenne ne sert que de repli lorsque la position n'est pas
 * exploitable : elle s'écarte de l'élongation réelle de plus de 8°, soit
 * plusieurs points d'illumination (voir ALGORITHME.md, section 6).
 */

#include "moon_phase.h"

#include <math.h>

#include "astro.h"
#include "moon_position.h"
#include "sun_position.h"

#define SYNODIC_MONTH 29.530588853
#define NEW_MOON_JD 2451550.1

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Fraction du cycle synodique moyen écoulée depuis la nouvelle lune de
// référence. Repli utilisé uniquement quand la position lunaire est
// indisponible.
static double synodic_phase_fraction(double julian_day) {
  double phase;

  phase = fmod(julian_day - NEW_MOON_JD, SYNODIC_MONTH) / SYNODIC_MONTH;
  if (phase < 0.0) {
    phase += 1.0;
  }

  return phase;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
// Élongation Lune-Soleil : fraction du cycle synodique (différence de
// longitudes écliptiques) et cosinus de l'écart angulaire réel, qui tient
// compte de la latitude lunaire. Renvoie 0 si la position n'est pas
// exploitable.
static int elongation(double julian_day, double *fraction, double *cosine) {
  MoonPosition moon;
  SunPosition sun;

  moon_position_at(julian_day, &moon);
  sun_position_at(julian_day, &sun);

  if (!isfinite(moon.ecliptic_longitude) || !isfinite(moon.ecliptic_latitude) ||
      !isfinite(sun.ecliptic_longitude)) {
    return 0;
  }

  const double difference =
      astro_norm_360(moon.ecliptic_longitude - sun.ecliptic_longitude);

  *fraction = difference / 360.0;
  // Le Soleil a une latitude écliptique nulle dans ce modèle.
  *cosine = astro_cosd(moon.ecliptic_latitude) * astro_cosd(difference);
  return 1;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// Détermine la phase lunaire (8 valeurs) à partir d'un Julian Day.
// Un Julian Day non fini est invalide : la phase est alors explicitement
// indéterminée (MOON_PHASE_INVALID) plutôt que silencieusement classée.
MoonPhase moon_phase_from_julian_day(double julian_day) {
  double fraction;
  double cosine;

  if (!isfinite(julian_day)) {
    return MOON_PHASE_INVALID;
  }

  if (!elongation(julian_day, &fraction, &cosine)) {
    fraction = synodic_phase_fraction(julian_day);
  }

  if (fraction < 0.03 || fraction > 0.97)
    return MOON_NEW;
  else if (fraction < 0.22)
    return MOON_WAXING_CRESCENT;
  else if (fraction < 0.28)
    return MOON_FIRST_QUARTER;
  else if (fraction < 0.47)
    return MOON_WAXING_GIBBOUS;
  else if (fraction < 0.53)
    return MOON_FULL;
  else if (fraction < 0.72)
    return MOON_WANING_GIBBOUS;
  else if (fraction < 0.78)
    return MOON_LAST_QUARTER;
  else
    return MOON_WANING_CRESCENT;
}

// Retourne la fraction éclairée du disque (0.0 = nouvelle, 1.0 = pleine).
// La fraction vaut (1 - cos(élongation)) / 2 : l'angle Soleil-Lune-Terre est le
// complément de l'élongation géocentrique, le Soleil étant quasi à l'infini.
double moon_illumination(double julian_day) {
  double fraction;
  double cosine;

  if (!isfinite(julian_day)) {
    return NAN;
  }

  if (elongation(julian_day, &fraction, &cosine)) {
    double illumination = (1.0 - cosine) / 2.0;

    if (illumination < 0.0) {
      return 0.0;
    }
    if (illumination > 1.0) {
      return 1.0;
    }

    return illumination;
  }

  const double phase = synodic_phase_fraction(julian_day);
  return (1.0 - cos(2.0 * M_PI * phase)) / 2.0;
}