/*
 * test_moon_duration.c
 * Tests du calcul de durée : lever/coucher, passage à minuit, cas polaires et
 * entrées invalides.
 */

#include "datetime.h"
#include "moon_duration.h"
#include "moon_events.h"
#include "moon_position.h"
#include "support/test_runner.h"

#include <math.h>

#define PARIS_LATITUDE 48.8566
#define PARIS_LONGITUDE 2.3522

// Compte les tranches passées au-dessus de l'horizon lunaire dans une journée.
static int count_above_horizon_intervals(double start_jd, double end_jd) {
  const int steps = 288;
  int intervals = 0;
  int previous_above =
      moon_altitude(start_jd, PARIS_LATITUDE, PARIS_LONGITUDE) >=
      MOON_HORIZON_ALTITUDE_DEG;

  if (previous_above) {
    intervals = 1;
  }

  for (int i = 1; i <= steps; i++) {
    const double jd = start_jd + (end_jd - start_jd) * (double)i / steps;
    const int above = moon_altitude(jd, PARIS_LATITUDE, PARIS_LONGITUDE) >=
                      MOON_HORIZON_ALTITUDE_DEG;

    if (above && !previous_above) {
      intervals++;
    }
    previous_above = above;
  }

  return intervals;
}

// Vérifie le calcul de durée pour Paris le 14 novembre 2026.
static void test_moon_duration_paris_november_14(void) {
  // Given
  const double day_start_jd = 2461358.5 - 1.0 / 24.0;
  const double day_end_jd = day_start_jd + 1.0;
  const double expected_rise_jd = 2461358.5 + 11.8628 / 24.0;
  const double expected_set_jd = 2461358.5 + 19.8975 / 24.0;
  const double expected_duration_hours = 8.03;
  const double tolerance_days = 0.003;
  const double tolerance_hours = 0.1;
  MoonDuration duration;

  // When
  int status = moon_duration_compute(day_start_jd, day_end_jd, PARIS_LATITUDE,
                                     PARIS_LONGITUDE, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_TRUE(duration.has_rise);
  TEST_ASSERT_TRUE(duration.has_set);
  TEST_ASSERT_FALSE(duration.all_day);
  TEST_ASSERT_FALSE(duration.none);
  TEST_ASSERT_DOUBLE_EQ(duration.rise_julian_day, expected_rise_jd,
                        tolerance_days);
  TEST_ASSERT_DOUBLE_EQ(duration.set_julian_day, expected_set_jd,
                        tolerance_days);
  TEST_ASSERT_DOUBLE_EQ(duration.duration_hours, expected_duration_hours,
                        tolerance_hours);
}

// Vérifie qu'une durée peut franchir minuit local.
static void test_moon_duration_crosses_local_midnight(void) {
  // Given
  const double day_start_jd = 2461361.5 - 1.0 / 24.0;
  const double day_end_jd = day_start_jd + 1.0;
  const double expected_rise_jd = 2461361.5 + 12.95 / 24.0;
  const double expected_set_jd = 2461361.5 + 23.3833 / 24.0;
  const double tolerance_days = 0.003;
  MoonDuration duration;

  // When
  int status = moon_duration_compute(day_start_jd, day_end_jd, PARIS_LATITUDE,
                                     PARIS_LONGITUDE, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_TRUE(duration.has_rise);
  TEST_ASSERT_TRUE(duration.has_set);
  TEST_ASSERT_DOUBLE_EQ(duration.rise_julian_day, expected_rise_jd,
                        tolerance_days);
  TEST_ASSERT_DOUBLE_EQ(duration.set_julian_day, expected_set_jd,
                        tolerance_days);
  TEST_ASSERT_TRUE(duration.set_julian_day > day_end_jd);
}

// Vérifie qu'une Lune sous l'horizon toute la journée donne une durée nulle.
static void test_moon_duration_returns_none_for_non_rising_moon(void) {
  // Given
  const double day_start_jd = 2461358.5 - 1.0 / 24.0;
  const double day_end_jd = day_start_jd + 1.0;
  MoonDuration duration;

  // When
  int status =
      moon_duration_compute(day_start_jd, day_end_jd, 89.0, 0.0, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_FALSE(duration.has_rise);
  TEST_ASSERT_FALSE(duration.has_set);
  TEST_ASSERT_FALSE(duration.all_day);
  TEST_ASSERT_TRUE(duration.none);
  TEST_ASSERT_DOUBLE_EQ(duration.duration_hours, 0.0, 1e-12);
}

// Vérifie qu'une Lune circumpolaire donne une durée de vingt-quatre heures.
static void test_moon_duration_returns_all_day_for_circumpolar_moon(void) {
  // Given
  const double day_start_jd = 2461368.5 - 1.0 / 24.0;
  const double day_end_jd = day_start_jd + 1.0;
  MoonDuration duration;

  // When
  int status =
      moon_duration_compute(day_start_jd, day_end_jd, 89.0, 0.0, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_FALSE(duration.has_rise);
  TEST_ASSERT_FALSE(duration.has_set);
  TEST_ASSERT_TRUE(duration.all_day);
  TEST_ASSERT_FALSE(duration.none);
  TEST_ASSERT_DOUBLE_EQ(duration.duration_hours, 24.0, 1e-12);
}

// Vérifie qu'une sortie nulle est refusée.
static void test_moon_duration_rejects_null_output(void) {
  // Given
  const double day_start_jd = 2461358.5 - 1.0 / 24.0;
  const double day_end_jd = day_start_jd + 1.0;

  // When
  int status = moon_duration_compute(day_start_jd, day_end_jd, PARIS_LATITUDE,
                                     PARIS_LONGITUDE, NULL);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie qu'un intervalle de journée invalide est refusé.
static void test_moon_duration_rejects_invalid_interval(void) {
  // Given
  const double day_start_jd = 2461358.5;
  const double day_end_jd = day_start_jd;
  MoonDuration duration;

  // When
  int status = moon_duration_compute(day_start_jd, day_end_jd, PARIS_LATITUDE,
                                     PARIS_LONGITUDE, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie qu'une borne non finie est refusée.
static void test_moon_duration_rejects_non_finite_bound(void) {
  // Given
  const double day_start_jd = 2461358.5;
  const double day_end_jd = day_start_jd + 1.0;
  MoonDuration duration;

  // When
  int status = moon_duration_compute(NAN, day_end_jd, PARIS_LATITUDE,
                                     PARIS_LONGITUDE, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie qu'une fenêtre plus large que la journée est refusée.
static void test_moon_duration_rejects_overlong_window(void) {
  // Given
  const double day_start_jd = 2461358.5;
  const double day_end_jd = day_start_jd + 10.0;
  MoonDuration duration;

  // When
  int status = moon_duration_compute(day_start_jd, day_end_jd, PARIS_LATITUDE,
                                     PARIS_LONGITUDE, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie qu'une journée à deux tranches conserve l'intervalle complet
// lever→coucher, et non la somme des tranches au-dessus de l'horizon.
static void test_moon_duration_keeps_full_interval_for_two_slice_day(void) {
  // Given
  const DateTime day = datetime_make(2026, 1, 1, 0, 0, 0);
  const double day_start_jd = datetime_to_julian(day);
  const double day_end_jd = day_start_jd + 1.0;
  const double expected_duration_hours = 17.74208825826645;
  const double tolerance_hours = 0.01;
  MoonDuration duration;

  // When
  const int status = moon_duration_compute(
      day_start_jd, day_end_jd, PARIS_LATITUDE, PARIS_LONGITUDE, &duration);

  // Then
  TEST_ASSERT_INT_EQ(count_above_horizon_intervals(day_start_jd, day_end_jd),
                     2);
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_TRUE(duration.has_rise);
  TEST_ASSERT_TRUE(duration.has_set);
  TEST_ASSERT_FALSE(duration.all_day);
  TEST_ASSERT_FALSE(duration.none);
  TEST_ASSERT_DOUBLE_EQ(duration.duration_hours, expected_duration_hours,
                        tolerance_hours);
}

// Vérifie qu'une Lune visible toute la journée est classée jour entier même
// lorsqu'un événement isolé existe hors de la fenêtre journalière.
static void test_moon_duration_classifies_all_day_with_out_of_window_event(
    void) {
  // Given
  const DateTime day = datetime_make(2026, 1, 7, 0, 0, 0);
  const double day_start_jd = datetime_to_julian(day);
  const double day_end_jd = day_start_jd + 1.0;
  MoonDuration duration;

  // When
  const int status =
      moon_duration_compute(day_start_jd, day_end_jd, 89.0, 0.0, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_FALSE(duration.has_rise);
  TEST_ASSERT_FALSE(duration.has_set);
  TEST_ASSERT_TRUE(duration.all_day);
  TEST_ASSERT_FALSE(duration.none);
  TEST_ASSERT_DOUBLE_EQ(duration.duration_hours, 24.0, 1e-12);
}

// Vérifie qu'une Lune absente toute la journée est classée Lune absente même
// lorsqu'un événement isolé existe hors de la fenêtre journalière.
static void test_moon_duration_classifies_none_with_out_of_window_event(void) {
  // Given
  const DateTime day = datetime_make(2026, 1, 10, 0, 0, 0);
  const double day_start_jd = datetime_to_julian(day);
  const double day_end_jd = day_start_jd + 1.0;
  MoonDuration duration;

  // When
  const int status =
      moon_duration_compute(day_start_jd, day_end_jd, 89.0, 0.0, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_FALSE(duration.has_rise);
  TEST_ASSERT_FALSE(duration.has_set);
  TEST_ASSERT_FALSE(duration.all_day);
  TEST_ASSERT_TRUE(duration.none);
  TEST_ASSERT_DOUBLE_EQ(duration.duration_hours, 0.0, 1e-12);
}

// Vérifie qu'un événement isolé dans la journée ne permet pas de calculer la
// durée : le comportement « Indisponible » est conservé.
static void test_moon_duration_keeps_unavailable_for_isolated_event(void) {
  // Given
  const DateTime day = datetime_make(2024, 1, 3, 0, 0, 0);
  const double day_start_jd = datetime_to_julian(day);
  const double day_end_jd = day_start_jd + 1.0;
  MoonDuration duration;

  // When
  const int status =
      moon_duration_compute(day_start_jd, day_end_jd, 89.0, 0.0, &duration);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_FALSE(duration.has_rise);
  TEST_ASSERT_FALSE(duration.has_set);
  TEST_ASSERT_FALSE(duration.all_day);
  TEST_ASSERT_FALSE(duration.none);
  TEST_ASSERT_DOUBLE_EQ(duration.duration_hours, 0.0, 1e-12);
}

static const TestCase test_cases[] = {
    {"test_moon_duration_paris_november_14",
     test_moon_duration_paris_november_14},
    {"test_moon_duration_crosses_local_midnight",
     test_moon_duration_crosses_local_midnight},
    {"test_moon_duration_returns_none_for_non_rising_moon",
     test_moon_duration_returns_none_for_non_rising_moon},
    {"test_moon_duration_returns_all_day_for_circumpolar_moon",
     test_moon_duration_returns_all_day_for_circumpolar_moon},
    {"test_moon_duration_classifies_all_day_with_out_of_window_event",
     test_moon_duration_classifies_all_day_with_out_of_window_event},
    {"test_moon_duration_classifies_none_with_out_of_window_event",
     test_moon_duration_classifies_none_with_out_of_window_event},
    {"test_moon_duration_keeps_unavailable_for_isolated_event",
     test_moon_duration_keeps_unavailable_for_isolated_event},
    {"test_moon_duration_keeps_full_interval_for_two_slice_day",
     test_moon_duration_keeps_full_interval_for_two_slice_day},
    {"test_moon_duration_rejects_null_output",
     test_moon_duration_rejects_null_output},
    {"test_moon_duration_rejects_invalid_interval",
     test_moon_duration_rejects_invalid_interval},
    {"test_moon_duration_rejects_non_finite_bound",
     test_moon_duration_rejects_non_finite_bound},
    {"test_moon_duration_rejects_overlong_window",
     test_moon_duration_rejects_overlong_window},
};

int main(void) {
  return runner_run("moon_duration", test_cases, TEST_CASE_COUNT(test_cases));
}