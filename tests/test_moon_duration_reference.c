/*
 * test_moon_duration_reference.c
 * Tests de précision face aux éphémérides JPL Horizons et cas de référence
 * multi-villes.
 */

#include <math.h>
#include <stddef.h>

#include "datetime.h"
#include "moon_duration.h"
#include "support/test_runner.h"

#define SECONDS_PER_DAY 86400.0
#define REFERENCE_TOLERANCE_DAYS 0.0009
#define REFERENCE_MAX_ERROR_SECONDS 80.0
#define REFERENCE_MEAN_ERROR_SECONDS 45.0

typedef struct {
  const char *name;
  int year;
  int month;
  int day;
  int utc_offset_hours;
  double latitude;
  double longitude;
  int rise_year;
  int rise_month;
  int rise_day;
  int rise_hour;
  int rise_minute;
  int rise_second;
  int set_year;
  int set_month;
  int set_day;
  int set_hour;
  int set_minute;
  int set_second;
} ReferenceCase;

// Heures RTS publiées par NASA/JPL Horizons (résolution d'une minute), couvrant
// plusieurs villes, saisons et années extrêmes (1900-3000).
static const ReferenceCase reference_cases[] = {
    {"Paris 2026-11-14", 2026, 11, 14, 1, 48.8566, 2.3522, 2026, 11, 14, 11, 52,
     0, 2026, 11, 14, 19, 55, 0},
    {"Paris 2026-11-15", 2026, 11, 15, 1, 48.8566, 2.3522, 2026, 11, 15, 12, 19,
     0, 2026, 11, 15, 21, 4, 0},
    {"Paris 2026-11-16", 2026, 11, 16, 1, 48.8566, 2.3522, 2026, 11, 16, 12, 40,
     0, 2026, 11, 16, 22, 14, 0},
    {"Paris 2026-11-17", 2026, 11, 17, 1, 48.8566, 2.3522, 2026, 11, 17, 12, 58,
     0, 2026, 11, 17, 23, 24, 0},
    {"New York 2026-11-14", 2026, 11, 14, -5, 40.7128, -74.0060, 2026, 11, 14,
     16, 30, 0, 2026, 11, 15, 1, 47, 0},
    {"New York 2026-11-15", 2026, 11, 15, -5, 40.7128, -74.0060, 2026, 11, 15,
     17, 2, 0, 2026, 11, 16, 2, 50, 0},
    {"New York 2026-11-16", 2026, 11, 16, -5, 40.7128, -74.0060, 2026, 11, 16,
     17, 29, 0, 2026, 11, 17, 3, 53, 0},
    {"New York 2026-11-17", 2026, 11, 17, -5, 40.7128, -74.0060, 2026, 11, 17,
     17, 53, 0, 2026, 11, 18, 4, 56, 0},
    {"Paris 2026-03-20 (UTC)", 2026, 3, 20, 0, 48.8566, 2.3522, 2026, 3, 20, 6,
     11, 0, 2026, 3, 20, 20, 19, 0},
    {"Paris 2026-06-21 (UTC)", 2026, 6, 21, 0, 48.8566, 2.3522, 2026, 6, 21, 11,
     31, 0, 2026, 6, 21, 23, 38, 0},
    {"Paris 2026-09-22 (UTC)", 2026, 9, 22, 0, 48.8566, 2.3522, 2026, 9, 22, 16,
     16, 0, 2026, 9, 23, 1, 38, 0},
    {"Paris 2026-12-21 (UTC)", 2026, 12, 21, 0, 48.8566, 2.3522, 2026, 12, 21,
     13, 13, 0, 2026, 12, 22, 6, 9, 0},
    {"New York 2026-03-20 (UTC)", 2026, 3, 20, 0, 40.7128, -74.006, 2026, 3, 20,
     11, 32, 0, 2026, 3, 21, 1, 25, 0},
    {"New York 2026-06-21 (UTC)", 2026, 6, 21, 0, 40.7128, -74.006, 2026, 6, 21,
     16, 50, 0, 2026, 6, 22, 4, 51, 0},
    {"New York 2026-09-22 (UTC)", 2026, 9, 22, 0, 40.7128, -74.006, 2026, 9, 21,
     20, 34, 0, 2026, 9, 22, 6, 14, 0},
    {"New York 2026-12-21 (UTC)", 2026, 12, 21, 0, 40.7128, -74.006, 2026, 12,
     20, 18, 20, 0, 2026, 12, 21, 9, 35, 0},
    {"Sydney 2026-03-20 (UTC)", 2026, 3, 20, 0, -33.8688, 151.2093, 2026, 3, 19,
     20, 56, 0, 2026, 3, 20, 8, 34, 0},
    {"Sydney 2026-06-21 (UTC)", 2026, 6, 21, 0, -33.8688, 151.2093, 2026, 6, 21,
     1, 25, 0, 2026, 6, 21, 13, 39, 0},
    {"Sydney 2026-09-22 (UTC)", 2026, 9, 22, 0, -33.8688, 151.2093, 2026, 9, 22,
     3, 24, 0, 2026, 9, 22, 17, 42, 0},
    {"Sydney 2026-12-21 (UTC)", 2026, 12, 21, 0, -33.8688, 151.2093, 2026, 12,
     21, 6, 6, 0, 2026, 12, 21, 16, 13, 0},
    {"Tromso 2026-06-21 (UTC)", 2026, 6, 21, 0, 69.6492, 18.9553, 2026, 6, 21,
     10, 18, 0, 2026, 6, 21, 22, 14, 0},
    {"Ushuaia 2026-06-21 (UTC)", 2026, 6, 21, 0, -54.8019, -68.303, 2026, 6, 21,
     16, 17, 0, 2026, 6, 22, 5, 7, 0},
    {"Ushuaia 2026-12-21 (UTC)", 2026, 12, 21, 0, -54.8019, -68.303, 2026, 12,
     20, 21, 33, 0, 2026, 12, 21, 5, 11, 0},
    {"Quito 2026-03-20 (UTC)", 2026, 3, 20, 0, 0.1807, -78.4678, 2026, 3, 20,
     12, 28, 0, 2026, 3, 21, 0, 52, 0},
    {"Quito 2026-09-22 (UTC)", 2026, 9, 22, 0, 0.1807, -78.4678, 2026, 9, 21,
     19, 28, 0, 2026, 9, 22, 7, 50, 0},
    {"Honolulu 2026-06-21 (UTC)", 2026, 6, 21, 0, 21.3069, -157.8583, 2026, 6,
     20, 21, 42, 0, 2026, 6, 21, 10, 9, 0},
    {"Honolulu 2026-12-21 (UTC)", 2026, 12, 21, 0, 21.3069, -157.8583, 2026, 12,
     21, 0, 49, 0, 2026, 12, 21, 14, 36, 0},
    {"Tokyo 2026-03-20 (UTC)", 2026, 3, 20, 0, 35.6762, 139.6503, 2026, 3, 19,
     21, 7, 0, 2026, 3, 20, 10, 19, 0},
    {"Cape Town 2026-06-21 (UTC)", 2026, 6, 21, 0, -33.9249, 18.4241, 2026, 6,
     21, 10, 26, 0, 2026, 6, 21, 22, 53, 0},
    {"Paris 1900-03-20 (UTC)", 1900, 3, 20, 0, 48.8566, 2.3522, 1900, 3, 19, 21,
     45, 0, 1900, 3, 20, 7, 11, 0},
    {"Paris 1950-06-21 (UTC)", 1950, 6, 21, 0, 48.8566, 2.3522, 1950, 6, 21, 9,
     41, 0, 1950, 6, 21, 23, 32, 0},
    {"Paris 2000-02-29 (UTC)", 2000, 2, 29, 0, 48.8566, 2.3522, 2000, 2, 29, 3,
     6, 0, 2000, 2, 29, 11, 56, 0},
    {"Paris 2050-04-10 (UTC)", 2050, 4, 10, 0, 48.8566, 2.3522, 2050, 4, 9, 21,
     13, 0, 2050, 4, 10, 6, 34, 0},
    {"Paris 2100-01-01 (UTC)", 2100, 1, 1, 0, 48.8566, 2.3522, 2099, 12, 31, 20,
     51, 0, 2100, 1, 1, 10, 38, 0},
    {"Paris 3000-01-01 (UTC)", 3000, 1, 1, 0, 48.8566, 2.3522, 3000, 1, 1, 9,
     12, 0, 3000, 1, 1, 20, 2, 0},
    {"Sydney 1900-03-20 (UTC)", 1900, 3, 20, 0, -33.8688, 151.2093, 1900, 3, 20,
     9, 59, 0, 1900, 3, 21, 0, 15, 0},
    {"Sydney 1950-06-21 (UTC)", 1950, 6, 21, 0, -33.8688, 151.2093, 1950, 6, 21,
     0, 57, 0, 1950, 6, 21, 12, 12, 0},
    {"Sydney 2000-02-29 (UTC)", 2000, 2, 29, 0, -33.8688, 151.2093, 2000, 2, 29,
     14, 51, 0, 2000, 3, 1, 5, 18, 0},
    {"Sydney 2050-04-10 (UTC)", 2050, 4, 10, 0, -33.8688, 151.2093, 2050, 4, 10,
     9, 24, 0, 2050, 4, 10, 23, 42, 0},
    {"Sydney 2100-01-01 (UTC)", 2100, 1, 1, 0, -33.8688, 151.2093, 2100, 1, 1,
     12, 33, 0, 2100, 1, 2, 0, 29, 0},
    {"Sydney 3000-01-01 (UTC)", 3000, 1, 1, 0, -33.8688, 151.2093, 2999, 12, 31,
     21, 35, 0, 3000, 1, 1, 10, 58, 0},
    {"New York 1900-03-20 (UTC)", 1900, 3, 20, 0, 40.7128, -74.006, 1900, 3, 20,
     2, 42, 0, 1900, 3, 20, 12, 48, 0},
    {"New York 1950-06-21 (UTC)", 1950, 6, 21, 0, 40.7128, -74.006, 1950, 6, 21,
     15, 14, 0, 1950, 6, 22, 4, 34, 0},
    {"New York 2000-02-29 (UTC)", 2000, 2, 29, 0, 40.7128, -74.006, 2000, 2, 29,
     7, 55, 0, 2000, 2, 29, 17, 39, 0},
    {"New York 2050-04-10 (UTC)", 2050, 4, 10, 0, 40.7128, -74.006, 2050, 4, 10,
     2, 9, 0, 2050, 4, 10, 12, 13, 0},
    {"New York 2100-01-01 (UTC)", 2100, 1, 1, 0, 40.7128, -74.006, 2100, 1, 1,
     2, 25, 0, 2100, 1, 1, 15, 40, 0},
    {"New York 3000-01-01 (UTC)", 3000, 1, 1, 0, 40.7128, -74.006, 3000, 1, 1,
     14, 12, 0, 3000, 1, 2, 1, 30, 0},
    {"Quito 1900-03-20 (UTC)", 1900, 3, 20, 0, 0.1807, -78.4678, 1900, 3, 20, 1,
     54, 0, 1900, 3, 20, 14, 17, 0},
    {"Quito 2000-02-29 (UTC)", 2000, 2, 29, 0, 0.1807, -78.4678, 2000, 2, 29, 6,
     54, 0, 2000, 2, 29, 19, 17, 0},
    {"Quito 2100-01-01 (UTC)", 2100, 1, 1, 0, 0.1807, -78.4678, 2100, 1, 1, 3,
     16, 0, 2100, 1, 1, 15, 39, 0},
    {"Quito 3000-01-01 (UTC)", 3000, 1, 1, 0, 0.1807, -78.4678, 3000, 1, 1, 13,
     56, 0, 3000, 1, 2, 2, 17, 0},
    {"Honolulu 1900-06-21 (UTC)", 1900, 6, 21, 0, 21.3069, -157.8583, 1900, 6,
     21, 11, 20, 0, 1900, 6, 22, 0, 26, 0},
    {"Honolulu 2000-12-21 (UTC)", 2000, 12, 21, 0, 21.3069, -157.8583, 2000, 12,
     21, 13, 37, 0, 2000, 12, 22, 1, 19, 0},
    {"Honolulu 2100-03-20 (UTC)", 2100, 3, 20, 0, 21.3069, -157.8583, 2100, 3,
     19, 22, 35, 0, 2100, 3, 20, 12, 39, 0},
    {"Honolulu 3000-09-22 (UTC)", 3000, 9, 22, 0, 21.3069, -157.8583, 3000, 9,
     21, 17, 52, 0, 3000, 9, 22, 5, 46, 0},
};

// Prépare la fenêtre UTC de la journée locale d'un cas de référence.
static void setup_reference_case(const ReferenceCase *test_case,
                                 double *day_start_jd, double *day_end_jd) {
  const DateTime local_midnight =
      datetime_make(test_case->year, test_case->month, test_case->day, 0, 0, 0);

  *day_start_jd = datetime_to_julian(local_midnight) -
                  (double)test_case->utc_offset_hours / 24.0;
  *day_end_jd = *day_start_jd + 1.0;
}

// Convertit un instant UTC de référence en Julian Day.
static double reference_event_jd(int year, int month, int day, int hour,
                                 int minute, int second) {
  return datetime_to_julian(
      datetime_make(year, month, day, hour, minute, second));
}

// Retourne le lever UTC attendu d'un cas de référence.
static double reference_rise_jd(const ReferenceCase *test_case) {
  return reference_event_jd(test_case->rise_year, test_case->rise_month,
                            test_case->rise_day, test_case->rise_hour,
                            test_case->rise_minute, test_case->rise_second);
}

// Retourne le coucher UTC attendu d'un cas de référence.
static double reference_set_jd(const ReferenceCase *test_case) {
  return reference_event_jd(test_case->set_year, test_case->set_month,
                            test_case->set_day, test_case->set_hour,
                            test_case->set_minute, test_case->set_second);
}

// Vérifie les levers et couchers calculés par rapport au cas de référence.
static void assert_reference_duration(const ReferenceCase *test_case,
                                      const MoonDuration *duration,
                                      double tolerance_days) {
  TEST_ASSERT_TRUE(duration->has_rise);
  TEST_ASSERT_TRUE(duration->has_set);
  TEST_ASSERT_DOUBLE_EQ(duration->rise_julian_day, reference_rise_jd(test_case),
                        tolerance_days);
  TEST_ASSERT_DOUBLE_EQ(duration->set_julian_day, reference_set_jd(test_case),
                        tolerance_days);
}

// Vérifie les heures RTS de JPL Horizons pour Paris le 14 novembre 2026.
static void test_reference_paris_november_14(void) {
  // Given
  const ReferenceCase *test_case = &reference_cases[0];
  double day_start_jd;
  double day_end_jd;
  MoonDuration duration;

  setup_reference_case(test_case, &day_start_jd, &day_end_jd);

  // When
  int status =
      moon_duration_compute(day_start_jd, day_end_jd, test_case->latitude,
                            test_case->longitude, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_reference_duration(test_case, &duration, REFERENCE_TOLERANCE_DAYS);
}

// Vérifie les heures RTS de JPL Horizons pour Paris le 17 novembre 2026,
// lorsque le coucher a lieu après minuit.
static void test_reference_paris_november_17_crosses_midnight(void) {
  // Given
  const ReferenceCase *test_case = &reference_cases[3];
  double day_start_jd;
  double day_end_jd;
  MoonDuration duration;

  setup_reference_case(test_case, &day_start_jd, &day_end_jd);

  // When
  int status =
      moon_duration_compute(day_start_jd, day_end_jd, test_case->latitude,
                            test_case->longitude, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_reference_duration(test_case, &duration, REFERENCE_TOLERANCE_DAYS);
  TEST_ASSERT_TRUE(duration.set_julian_day > day_end_jd);
}

// Vérifie les heures RTS de JPL Horizons pour New York le 14 novembre 2026.
static void test_reference_new_york_november_14(void) {
  // Given
  const ReferenceCase *test_case = &reference_cases[4];
  double day_start_jd;
  double day_end_jd;
  MoonDuration duration;

  setup_reference_case(test_case, &day_start_jd, &day_end_jd);

  // When
  int status =
      moon_duration_compute(day_start_jd, day_end_jd, test_case->latitude,
                            test_case->longitude, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_reference_duration(test_case, &duration, REFERENCE_TOLERANCE_DAYS);
}

// Vérifie la précision globale par rapport aux références JPL Horizons.
static void test_reference_precision_matches_jpl_horizons(void) {
  // Given
  const size_t count = TEST_CASE_COUNT(reference_cases);
  double total_error_seconds = 0.0;
  double max_error_seconds = 0.0;
  int event_count = 0;

  // When
  for (size_t i = 0; i < count; i++) {
    const ReferenceCase *test_case = &reference_cases[i];
    double day_start_jd;
    double day_end_jd;
    MoonDuration duration;

    setup_reference_case(test_case, &day_start_jd, &day_end_jd);
    int status =
        moon_duration_compute(day_start_jd, day_end_jd, test_case->latitude,
                              test_case->longitude, &duration);
    TEST_ASSERT_INT_EQ(status, 0);
    TEST_ASSERT_TRUE(duration.has_rise);
    TEST_ASSERT_TRUE(duration.has_set);

    const double rise_error_seconds =
        fabs(duration.rise_julian_day - reference_rise_jd(test_case)) *
        SECONDS_PER_DAY;
    const double set_error_seconds =
        fabs(duration.set_julian_day - reference_set_jd(test_case)) *
        SECONDS_PER_DAY;

    total_error_seconds += rise_error_seconds + set_error_seconds;
    event_count += 2;

    if (rise_error_seconds > max_error_seconds) {
      max_error_seconds = rise_error_seconds;
    }
    if (set_error_seconds > max_error_seconds) {
      max_error_seconds = set_error_seconds;
    }
    if (rise_error_seconds > REFERENCE_MAX_ERROR_SECONDS ||
        set_error_seconds > REFERENCE_MAX_ERROR_SECONDS) {
      runner_context_info("cas %s : erreur lever=%.1f s, coucher=%.1f s",
                          test_case->name, rise_error_seconds,
                          set_error_seconds);
    }
  }

  const double mean_error_seconds = total_error_seconds / (double)event_count;

  // Then
  runner_context_info(
      "précision estimée : MAE=%.1f s, max=%.1f s (%d événements)",
      mean_error_seconds, max_error_seconds, event_count);
  TEST_ASSERT_TRUE(mean_error_seconds <= REFERENCE_MEAN_ERROR_SECONDS);
  TEST_ASSERT_TRUE(max_error_seconds <= REFERENCE_MAX_ERROR_SECONDS);
}

static const TestCase test_cases[] = {
    {"test_reference_paris_november_14", test_reference_paris_november_14},
    {"test_reference_paris_november_17_crosses_midnight",
     test_reference_paris_november_17_crosses_midnight},
    {"test_reference_new_york_november_14",
     test_reference_new_york_november_14},
    {"test_reference_precision_matches_jpl_horizons",
     test_reference_precision_matches_jpl_horizons},
};

int main(void) {
  return runner_run("moon_duration_reference", test_cases,
                    TEST_CASE_COUNT(test_cases));
}