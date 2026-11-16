/*
 * moon_duration.c
 * Sélection de l'intervalle lever/coucher qui chevauche la journée locale et
 * calcul de sa durée.
 */

#include "moon_duration.h"

#include <math.h>
#include <stddef.h>

#include "moon_events.h"
#include "moon_position.h"

// Capacite dimensionnee pour une fenetre de deux jours (une dizaine
// d'evenements) ; MAX_DURATION_WINDOW_DAYS garantit qu'elle ne peut pas etre
// atteinte.
#define EVENT_CAPACITY 32
#define SCAN_MARGIN_DAYS 2.0
#define MAX_DURATION_WINDOW_DAYS 2

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
// Retourne le nombre de jours d'intersection entre deux intervalles.
static double overlap_days(double interval_start, double interval_end,
                           double day_start, double day_end) {
  const double from = (interval_start > day_start) ? interval_start : day_start;
  const double to = (interval_end < day_end) ? interval_end : day_end;

  return (to > from) ? (to - from) : 0.0;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// Classe une journée sans événement interne : la Lune reste d'un côté de
// l'horizon. On échantillonne le début, le milieu et la fin de la journée
// pour éviter qu'une unique évaluation au début ne tranche à tort lorsque la
// Lune frôle l'horizon sur tout l'intervalle.
static void classify_day_without_events(double day_start_jd, double day_end_jd,
                                        double latitude, double longitude,
                                        MoonDuration *out) {
  const double start_altitude =
      moon_altitude(day_start_jd, latitude, longitude);
  const double middle_altitude =
      moon_altitude(0.5 * (day_start_jd + day_end_jd), latitude, longitude);
  const double end_altitude = moon_altitude(day_end_jd, latitude, longitude);

  // La Lune est au-dessus de l'horizon si tous les échantillons le sont.
  // En cas de désaccord (ce qui ne devrait pas arriver sans événement), on
  // se rabat sur l'altitude de début, qui classe la journée de façon stable.
  const int all_above = start_altitude > MOON_HORIZON_ALTITUDE_DEG &&
                        middle_altitude > MOON_HORIZON_ALTITUDE_DEG &&
                        end_altitude > MOON_HORIZON_ALTITUDE_DEG;
  const int all_below = start_altitude <= MOON_HORIZON_ALTITUDE_DEG &&
                        middle_altitude <= MOON_HORIZON_ALTITUDE_DEG &&
                        end_altitude <= MOON_HORIZON_ALTITUDE_DEG;

  if (all_above) {
    out->all_day = 1;
    out->none = 0;
    out->duration_hours = 24.0;
  } else if (all_below) {
    out->all_day = 0;
    out->none = 1;
    out->duration_hours = 0.0;
  } else {
    // Cas limite : la Lune frôle l'horizon. L'altitude de début tranche.
    out->all_day = start_altitude > MOON_HORIZON_ALTITUDE_DEG;
    out->none = !out->all_day;
    out->duration_hours = out->all_day ? 24.0 : 0.0;
  }
}

int moon_duration_compute(double day_start_jd, double day_end_jd,
                          double latitude, double longitude,
                          MoonDuration *out) {
  MoonEvent events[EVENT_CAPACITY];
  double best_overlap = 0.0;
  int count;

  if (out == NULL || !(day_end_jd > day_start_jd) || !isfinite(day_start_jd) ||
      !isfinite(day_end_jd) || !isfinite(latitude) || !isfinite(longitude)) {
    return -1;
  }

  out->has_rise = 0;
  out->has_set = 0;
  out->all_day = 0;
  out->none = 0;
  out->rise_julian_day = 0.0;
  out->set_julian_day = 0.0;
  out->duration_hours = 0.0;

  if (day_end_jd - day_start_jd > MAX_DURATION_WINDOW_DAYS) {
    return -1;
  }

  count = moon_events_scan(day_start_jd - SCAN_MARGIN_DAYS,
                           day_end_jd + SCAN_MARGIN_DAYS, latitude, longitude,
                           events, EVENT_CAPACITY);
  // Une entrée invalide ou un tampon tronqué rend la durée inutilisable.
  if (count < 0) {
    return -1;
  }

  if (count == 0) {
    // Sans croisement, la Lune reste d'un côté de l'horizon.
    classify_day_without_events(day_start_jd, day_end_jd, latitude, longitude,
                                out);
    return 0;
  }

  for (int i = 0; i < count; i++) {
    if (events[i].type != MOON_EVENT_RISE) {
      continue;
    }

    for (int j = i + 1; j < count; j++) {
      if (events[j].type != MOON_EVENT_SET) {
        continue;
      }

      const double overlap = overlap_days(
          events[i].julian_day, events[j].julian_day, day_start_jd, day_end_jd);
      if (overlap > best_overlap) {
        best_overlap = overlap;
        out->has_rise = 1;
        out->has_set = 1;
        out->rise_julian_day = events[i].julian_day;
        out->set_julian_day = events[j].julian_day;
        out->duration_hours =
            (events[j].julian_day - events[i].julian_day) * 24.0;
      }
      break;
    }
  }

  // Aucun couple retenu : s'il n'y a pas non plus d'événement dans la journée,
  // les événements du balayage sont tous hors bornes (cas polaires à lisière).
  // La Lune est alors entièrement visible ou entièrement absente.
  if (!out->has_rise || !out->has_set) {
    int has_event_in_day = 0;

    for (int i = 0; i < count; i++) {
      if (events[i].julian_day >= day_start_jd &&
          events[i].julian_day <= day_end_jd) {
        has_event_in_day = 1;
        break;
      }
    }

    if (!has_event_in_day) {
      classify_day_without_events(day_start_jd, day_end_jd, latitude, longitude,
                                  out);
    }
  }

  return 0;
}