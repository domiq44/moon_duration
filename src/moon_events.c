/*
 * moon_events.c
 * Détection des changements de signe de l'altitude lunaire au voisinage de
 * l'horizon, puis affinage par dichotomie.
 */

#include "moon_events.h"

#include <math.h>
#include <stddef.h>

#include "moon_position.h"

#define SCAN_STEP_DAYS (5.0 / 1440.0)                 // ~5 minutes
#define SCAN_STEP_NEAR_HORIZON_DAYS (30.0 / 86400.0)  // 30 secondes
// Marge au voisinage de l'horizon : elle couvre la variation maximale de
// l'altitude lunaire sur un pas grossier (environ 1,21° en 5 min), garantissant
// qu'aucun croisement n'est manqué entre deux pas.
#define HORIZON_MARGIN_DEG 1.3
#define BISECTION_STEPS 60

// Retourne l'altitude lunaire diminuée de l'altitude de l'horizon.
static double altitude_above_horizon(double julian_day, double latitude,
                                     double longitude) {
  return moon_altitude(julian_day, latitude, longitude) -
         MOON_HORIZON_ALTITUDE_DEG;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
// Affine un encadrement de lever/coucher par dichotomie.
static double refine_crossing(double low, double high, double latitude,
                              double longitude) {
  double low_value = altitude_above_horizon(low, latitude, longitude);

  for (int i = 0; i < BISECTION_STEPS; i++) {
    const double middle = 0.5 * (low + high);
    const double middle_value =
        altitude_above_horizon(middle, latitude, longitude);

    if ((low_value <= 0.0 && middle_value <= 0.0) ||
        (low_value >= 0.0 && middle_value >= 0.0)) {
      low = middle;
      low_value = middle_value;
    } else {
      high = middle;
    }
  }

  return 0.5 * (low + high);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

int moon_events_scan(double start_jd, double end_jd, double latitude,
                     double longitude, MoonEvent *events, int capacity) {
  double previous;
  double current_jd;
  int count = 0;
  int truncated = 0;

  if (events == NULL || capacity <= 0 || !(end_jd > start_jd) ||
      !isfinite(start_jd) || !isfinite(end_jd) || !isfinite(latitude) ||
      !isfinite(longitude)) {
    return -1;
  }

  previous = altitude_above_horizon(start_jd, latitude, longitude);
  current_jd = start_jd;

  while (current_jd < end_jd) {
    // Le pas se resserre au voisinage de l'horizon pour ne pas manquer un
    // croisement bref. La marge couvre la variation d'altitude du satellite
    // sur un pas grossier (environ 1,2° en 5 min au maximum).
    const double step = (fabs(previous) < HORIZON_MARGIN_DEG)
                            ? SCAN_STEP_NEAR_HORIZON_DAYS
                            : SCAN_STEP_DAYS;
    const double next_jd =
        (current_jd + step < end_jd) ? current_jd + step : end_jd;
    const double current = altitude_above_horizon(next_jd, latitude, longitude);

    if ((previous <= 0.0 && current > 0.0) ||
        (previous > 0.0 && current <= 0.0)) {
      if (count >= capacity) {
        // Le balayage continue pour détecter l'éventuel débordement : une
        // troncature silencieuse donnerait une durée fausse sans le signaler.
        truncated = 1;
      } else {
        events[count].type =
            (previous <= 0.0) ? MOON_EVENT_RISE : MOON_EVENT_SET;
        events[count].julian_day =
            refine_crossing(current_jd, next_jd, latitude, longitude);
        count++;
      }
    }

    previous = current;
    current_jd = next_jd;
  }

  return truncated ? MOON_EVENTS_TRUNCATED : count;
}