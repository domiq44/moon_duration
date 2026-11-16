/*
 * web_api.c
 * Exports WebAssembly de l'API web : jour julien, durée, événements, phase,
 * illumination, altitude, orientation et rendu ASCII.
 */

#include "web_api.h"

#include <math.h>
#include <stddef.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define WEB_API_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define WEB_API_EXPORT
#endif

#include "datetime.h"
#include "moon_ascii.h"
#include "moon_duration.h"
#include "moon_orientation.h"
#include "moon_phase.h"
#include "moon_position.h"

#define MOON_WEB_MIN_YEAR 1900
#define MOON_WEB_MAX_YEAR 3000
#define MOON_WEB_MIN_OFFSET_MINUTES (-1440.0)
#define MOON_WEB_MAX_OFFSET_MINUTES 1440.0

// Valeur « indéterminée » de l'API web : un NaN explicitement typé double.
// Passer par une constante nommée évite la promotion d'un NAN flottant dans une
// expression double et rend le contrat lisible pour les appelants (le NaN est
// la seule valeur qui signifie « pas d'événement » ou « entrée invalide »).
#define MOON_WEB_UNDEFINED ((double)NAN)

static int valid_date(int year, int month, int day) {
  const DateTime date = datetime_make(year, month, day, 0, 0, 0);

  return year >= MOON_WEB_MIN_YEAR && year <= MOON_WEB_MAX_YEAR &&
         datetime_is_valid(date);
}

static int valid_location(double latitude, double longitude) {
  return isfinite(latitude) && latitude >= -90.0 && latitude <= 90.0 &&
         isfinite(longitude) && longitude >= -180.0 && longitude <= 180.0;
}

static int valid_offset(double timezone_offset_minutes) {
  return isfinite(timezone_offset_minutes) &&
         timezone_offset_minutes >= MOON_WEB_MIN_OFFSET_MINUTES &&
         timezone_offset_minutes <= MOON_WEB_MAX_OFFSET_MINUTES;
}

static int valid_hour(int hour) { return hour >= 0 && hour <= 24; }

static int valid_minute(int minute) { return minute >= 0 && minute <= 59; }

// Valide une date et une heure locale pour l'aperçu.
static int valid_preview(int year, int month, int day, int hour, int minute,
                         double timezone_offset_minutes) {
  return valid_date(year, month, day) &&
         valid_offset(timezone_offset_minutes) && valid_hour(hour) &&
         valid_minute(minute);
}

// Julian Day d'une date et d'une heure locale.
static double preview_julian_day(int year, int month, int day, int hour,
                                 int minute, double timezone_offset_minutes) {
  return moon_web_julian_day(year, month, day, hour, timezone_offset_minutes) +
         (double)minute / 1440.0;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static int compute_duration_window(int year, int month, int day,
                                   double latitude_degrees,
                                   double longitude_degrees,
                                   double start_offset_minutes,
                                   double end_offset_minutes,
                                   MoonDuration *duration) {
  const double day_start_jd =
      moon_web_julian_day(year, month, day, 0, start_offset_minutes);
  const double day_end_jd =
      moon_web_julian_day(year, month, day, 24, end_offset_minutes);

  if (!isfinite(day_start_jd) || !isfinite(day_end_jd)) {
    return -1;
  }

  return moon_duration_compute(day_start_jd, day_end_jd, latitude_degrees,
                               longitude_degrees, duration);
}

static int compute_duration(int year, int month, int day,
                            double latitude_degrees, double longitude_degrees,
                            double timezone_offset_minutes,
                            MoonDuration *duration) {
  return compute_duration_window(year, month, day, latitude_degrees,
                                 longitude_degrees, timezone_offset_minutes,
                                 timezone_offset_minutes, duration);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

WEB_API_EXPORT double moon_web_julian_day(int year, int month, int day,
                                          int hour,
                                          double timezone_offset_minutes) {
  const DateTime local_midnight = datetime_make(year, month, day, 0, 0, 0);

  if (!valid_date(year, month, day) || !valid_offset(timezone_offset_minutes) ||
      !valid_hour(hour)) {
    return MOON_WEB_UNDEFINED;
  }

  return datetime_to_julian(local_midnight) + (double)hour / 24.0 -
         timezone_offset_minutes / 1440.0;
}

WEB_API_EXPORT double moon_web_duration_hours(int year, int month, int day,
                                              double latitude_degrees,
                                              double longitude_degrees,
                                              double timezone_offset_minutes) {
  MoonDuration duration;

  if (!valid_date(year, month, day) ||
      !valid_location(latitude_degrees, longitude_degrees) ||
      !valid_offset(timezone_offset_minutes)) {
    return MOON_WEB_UNDEFINED;
  }

  if (compute_duration(year, month, day, latitude_degrees, longitude_degrees,
                       timezone_offset_minutes, &duration) != 0 ||
      !duration.has_rise || !duration.has_set) {
    return MOON_WEB_UNDEFINED;
  }

  return duration.duration_hours;
}

WEB_API_EXPORT int moon_web_duration_mode(int year, int month, int day,
                                          double latitude_degrees,
                                          double longitude_degrees,
                                          double start_offset_minutes,
                                          double end_offset_minutes) {
  MoonDuration duration;

  if (!valid_date(year, month, day) ||
      !valid_location(latitude_degrees, longitude_degrees) ||
      !valid_offset(start_offset_minutes) ||
      !valid_offset(end_offset_minutes)) {
    return -1;
  }

  if (compute_duration_window(year, month, day, latitude_degrees,
                              longitude_degrees, start_offset_minutes,
                              end_offset_minutes, &duration) != 0) {
    return -1;
  }

  if (duration.all_day) {
    return 1;
  }
  if (duration.none) {
    return 2;
  }
  return (duration.has_rise && duration.has_set) ? 0 : -1;
}

WEB_API_EXPORT double moon_web_event_julian_day(int year, int month, int day,
                                                double latitude_degrees,
                                                double longitude_degrees,
                                                double timezone_offset_minutes,
                                                int is_rise) {
  MoonDuration duration;

  if (!valid_date(year, month, day) ||
      !valid_location(latitude_degrees, longitude_degrees) ||
      !valid_offset(timezone_offset_minutes) ||
      (is_rise != 0 && is_rise != 1)) {
    return MOON_WEB_UNDEFINED;
  }

  if (compute_duration(year, month, day, latitude_degrees, longitude_degrees,
                       timezone_offset_minutes, &duration) != 0) {
    return MOON_WEB_UNDEFINED;
  }

  if (is_rise) {
    return duration.has_rise ? duration.rise_julian_day : MOON_WEB_UNDEFINED;
  }

  return duration.has_set ? duration.set_julian_day : MOON_WEB_UNDEFINED;
}

WEB_API_EXPORT double moon_web_window(int year, int month, int day,
                                      double latitude_degrees,
                                      double longitude_degrees,
                                      double start_offset_minutes,
                                      double end_offset_minutes,
                                      double *rise_julian_day,
                                      double *set_julian_day) {
  MoonDuration duration;

  if (rise_julian_day == NULL || set_julian_day == NULL ||
      !valid_date(year, month, day) ||
      !valid_location(latitude_degrees, longitude_degrees) ||
      !valid_offset(start_offset_minutes) ||
      !valid_offset(end_offset_minutes)) {
    return MOON_WEB_UNDEFINED;
  }

  const double day_start_jd =
      moon_web_julian_day(year, month, day, 0, start_offset_minutes);
  const double day_end_jd =
      moon_web_julian_day(year, month, day, 24, end_offset_minutes);

  if (!isfinite(day_start_jd) || !isfinite(day_end_jd) ||
      moon_duration_compute(day_start_jd, day_end_jd, latitude_degrees,
                            longitude_degrees, &duration) != 0 ||
      !duration.has_rise || !duration.has_set) {
    return MOON_WEB_UNDEFINED;
  }

  *rise_julian_day = duration.rise_julian_day;
  *set_julian_day = duration.set_julian_day;

  return (duration.set_julian_day - duration.rise_julian_day) * 24.0;
}

WEB_API_EXPORT double moon_web_phase(int year, int month, int day,
                                     double timezone_offset_minutes) {
  return moon_web_phase_at(year, month, day, 12, 0, timezone_offset_minutes);
}

WEB_API_EXPORT double moon_web_phase_at(int year, int month, int day, int hour,
                                        int minute,
                                        double timezone_offset_minutes) {
  if (!valid_preview(year, month, day, hour, minute, timezone_offset_minutes)) {
    return MOON_WEB_UNDEFINED;
  }

  const double preview_jd = preview_julian_day(year, month, day, hour, minute,
                                               timezone_offset_minutes);

  if (!isfinite(preview_jd)) {
    return MOON_WEB_UNDEFINED;
  }

  return (double)moon_phase_from_julian_day(preview_jd);
}

WEB_API_EXPORT double moon_web_illumination(int year, int month, int day,
                                            double timezone_offset_minutes) {
  return moon_web_illumination_at(year, month, day, 12, 0,
                                  timezone_offset_minutes);
}

WEB_API_EXPORT double moon_web_illumination_at(int year, int month, int day,
                                               int hour, int minute,
                                               double timezone_offset_minutes) {
  if (!valid_preview(year, month, day, hour, minute, timezone_offset_minutes)) {
    return MOON_WEB_UNDEFINED;
  }

  const double preview_jd = preview_julian_day(year, month, day, hour, minute,
                                               timezone_offset_minutes);

  if (!isfinite(preview_jd)) {
    return MOON_WEB_UNDEFINED;
  }

  return moon_illumination(preview_jd);
}

WEB_API_EXPORT double moon_web_altitude(double julian_day,
                                        double latitude_degrees,
                                        double longitude_degrees) {
  if (!isfinite(julian_day) ||
      !valid_location(latitude_degrees, longitude_degrees)) {
    return MOON_WEB_UNDEFINED;
  }

  return moon_altitude(julian_day, latitude_degrees, longitude_degrees);
}

/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) */
WEB_API_EXPORT double moon_web_bright_limb_angle(
    int year, int month, int day, double latitude_degrees,
    double longitude_degrees, double timezone_offset_minutes) {
  return moon_web_bright_limb_angle_at(year, month, day, 12, 0,
                                       latitude_degrees, longitude_degrees,
                                       timezone_offset_minutes);
}

/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) */
WEB_API_EXPORT double moon_web_bright_limb_angle_at(
    int year, int month, int day, int hour, int minute, double latitude_degrees,
    double longitude_degrees, double timezone_offset_minutes) {
  if (!valid_preview(year, month, day, hour, minute, timezone_offset_minutes) ||
      !valid_location(latitude_degrees, longitude_degrees)) {
    return MOON_WEB_UNDEFINED;
  }

  const double preview_jd = preview_julian_day(year, month, day, hour, minute,
                                               timezone_offset_minutes);

  if (!isfinite(preview_jd)) {
    return MOON_WEB_UNDEFINED;
  }

  return moon_bright_limb_angle(preview_jd, latitude_degrees,
                                longitude_degrees);
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
WEB_API_EXPORT int moon_web_ascii_size(void) {
  return (MOON_ASCII_WIDTH + 1) * MOON_ASCII_HEIGHT + 1;
}

WEB_API_EXPORT int moon_web_ascii_into(int phase, double illumination,
                                       double bright_limb_angle_deg, char *out,
                                       int capacity) {
  char rows[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1];
  MoonAsciiState state;
  int offset = 0;

  if (out == NULL || capacity < moon_web_ascii_size()) {
    return -1;
  }

  out[0] = '\0';

  if (phase < 0 || phase > 7 || !isfinite(illumination)) {
    return -1;
  }

  if (illumination < 0.0) {
    illumination = 0.0;
  } else if (illumination > 1.0) {
    illumination = 1.0;
  }

  state.phase = (MoonPhase)phase;
  state.illumination = illumination;
  state.has_bright_limb_angle = isfinite(bright_limb_angle_deg);
  state.bright_limb_angle_deg = bright_limb_angle_deg;
  moon_ascii_draw(rows, state);

  for (int row = 0; row < MOON_ASCII_HEIGHT; row++) {
    for (int column = 0; column < MOON_ASCII_WIDTH; column++) {
      out[offset++] = rows[row][column];
    }
    out[offset++] = '\n';
  }
  out[offset] = '\0';

  return offset;
}
// NOLINTEND(bugprone-easily-swappable-parameters)