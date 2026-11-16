/*
 * test_datetime.c
 * Tests de la représentation DateTime : conversions Julian Day, calendriers
 * julien et grégorien, années bissextiles.
 */

#include "datetime.h"
#include "support/test_runner.h"

#include <limits.h>
#include <math.h>

// Vérifie que deux dates coïncident champ par champ.
static void assert_datetime_equals(DateTime expected, DateTime actual) {
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
  TEST_ASSERT_INT_EQ(actual.hour, expected.hour);
  TEST_ASSERT_INT_EQ(actual.minute, expected.minute);
  TEST_ASSERT_INT_EQ(actual.second, expected.second);
}

// Vérifie que l'époque J2000 est convertie en JD 2451545.0.
static void test_j2000_epoch(void) {
  // Given
  const DateTime datetime = datetime_make(2000, 1, 1, 12, 0, 0);
  const double expected_jd = 2451545.0;
  const double tolerance = 0.1;

  // When
  double jd = datetime_to_julian(datetime);

  // Then
  TEST_ASSERT_DOUBLE_EQ(jd, expected_jd, tolerance);
}

// Vérifie la conversion d'une date grégorienne connue.
// 1er janvier 2000 à 00:00 UTC correspond à JD 2451544.5.
static void test_gregorian_date(void) {
  // Given
  const DateTime datetime = datetime_make(2000, 1, 1, 0, 0, 0);
  const double expected_jd = 2451544.5;
  const double tolerance = 0.1;

  // When
  double jd = datetime_to_julian(datetime);

  // Then
  TEST_ASSERT_DOUBLE_EQ(jd, expected_jd, tolerance);
}

// Vérifie la conversion d'une date située après un 29 février.
// 1er mars 2024 à 00:00 UTC correspond à JD 2460370.5.
static void test_leap_year(void) {
  // Given
  const DateTime datetime = datetime_make(2024, 3, 1, 0, 0, 0);
  const double expected_jd = 2460370.5;
  const double tolerance = 0.1;

  // When
  double jd = datetime_to_julian(datetime);

  // Then
  TEST_ASSERT_DOUBLE_EQ(jd, expected_jd, tolerance);
}

// Vérifie la conversion d'une date dans une année non bissextile.
// 1er mars 2023 à 00:00 UTC correspond à JD 2460004.5.
static void test_non_leap_year(void) {
  // Given
  const DateTime datetime = datetime_make(2023, 3, 1, 0, 0, 0);
  const double expected_jd = 2460004.5;
  const double tolerance = 0.1;

  // When
  double jd = datetime_to_julian(datetime);

  // Then
  TEST_ASSERT_DOUBLE_EQ(jd, expected_jd, tolerance);
}

// Vérifie que le 15 juin 2025 est un dimanche.
static void test_weekday_returns_sunday_for_2025_06_15(void) {
  // Given
  const DateTime datetime = datetime_make(2025, 6, 15, 0, 0, 0);
  const int expected_weekday = 0;

  // When
  int weekday = datetime_weekday(datetime);

  // Then
  TEST_ASSERT_INT_EQ(weekday, expected_weekday);
}

// Vérifie que le 1er janvier 2000 est un samedi.
static void test_weekday_returns_saturday_for_2000_01_01(void) {
  // Given
  const DateTime datetime = datetime_make(2000, 1, 1, 0, 0, 0);
  const int expected_weekday = 6;

  // When
  int weekday = datetime_weekday(datetime);

  // Then
  TEST_ASSERT_INT_EQ(weekday, expected_weekday);
}

// Vérifie que le 6 janvier 2025 est un lundi.
static void test_weekday_returns_monday_for_2025_01_06(void) {
  // Given
  const DateTime datetime = datetime_make(2025, 1, 6, 0, 0, 0);
  const int expected_weekday = 1;

  // When
  int weekday = datetime_weekday(datetime);

  // Then
  TEST_ASSERT_INT_EQ(weekday, expected_weekday);
}

// Vérifie que le 29 février 2024 est un jeudi.
static void test_weekday_returns_thursday_for_leap_day_2024_02_29(void) {
  // Given
  const DateTime datetime = datetime_make(2024, 2, 29, 0, 0, 0);
  const int expected_weekday = 4;

  // When
  int weekday = datetime_weekday(datetime);

  // Then
  TEST_ASSERT_INT_EQ(weekday, expected_weekday);
}

// Vérifie qu'une date et une heure valides sont acceptées.
static void test_datetime_is_valid_for_valid_date(void) {
  // Given
  const DateTime datetime = datetime_make(2025, 6, 15, 14, 30, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_TRUE(valid);
}

// Vérifie qu'un mois hors de 1..12 est refusé.
static void test_datetime_is_valid_rejects_invalid_month(void) {
  // Given
  const DateTime datetime = datetime_make(2025, 13, 1, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_FALSE(valid);
}

// Vérifie qu'un jour inexistant dans le mois est refusé.
static void test_datetime_is_valid_rejects_invalid_day(void) {
  // Given
  const DateTime datetime = datetime_make(2025, 2, 30, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_FALSE(valid);
}

// Vérifie que le 29 février est accepté une année bissextile.
static void test_datetime_is_valid_accepts_leap_day(void) {
  // Given
  const DateTime datetime = datetime_make(2024, 2, 29, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_TRUE(valid);
}

// Vérifie que le 29 février est refusé une année non bissextile.
static void test_datetime_is_valid_rejects_non_leap_day(void) {
  // Given
  const DateTime datetime = datetime_make(2023, 2, 29, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_FALSE(valid);
}

// Vérifie que les composantes horaires hors plage sont refusées.
static void test_datetime_is_valid_rejects_invalid_time_components(void) {
  // Given
  const DateTime invalid_hour = datetime_make(2025, 6, 15, 24, 0, 0);
  const DateTime invalid_minute = datetime_make(2025, 6, 15, 0, 60, 0);
  const DateTime invalid_second = datetime_make(2025, 6, 15, 0, 0, 60);

  // When
  int hour_valid = datetime_is_valid(invalid_hour);
  int minute_valid = datetime_is_valid(invalid_minute);
  int second_valid = datetime_is_valid(invalid_second);

  // Then
  TEST_ASSERT_FALSE(hour_valid);
  TEST_ASSERT_FALSE(minute_valid);
  TEST_ASSERT_FALSE(second_valid);
}

// Vérifie qu'une date hors de 1..9999 est refusée.
static void test_datetime_is_valid_rejects_invalid_year(void) {
  // Given
  const DateTime datetime = datetime_make(0, 1, 1, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_FALSE(valid);
}

// Vérifie qu'une année supérieure à 9999 est refusée.
static void test_datetime_is_valid_rejects_year_above_max(void) {
  // Given
  const DateTime datetime = datetime_make(10000, 1, 1, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_FALSE(valid);
}

// Vérifie qu'un mois nul est refusé.
static void test_datetime_is_valid_rejects_month_zero(void) {
  // Given
  const DateTime datetime = datetime_make(2025, 0, 1, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_FALSE(valid);
}

// Vérifie qu'un jour nul est refusé.
static void test_datetime_is_valid_rejects_day_zero(void) {
  // Given
  const DateTime datetime = datetime_make(2025, 1, 0, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_FALSE(valid);
}

// Vérifie la conversion d'une date grégorienne de novembre 1582.
static void test_gregorian_date_in_november_1582(void) {
  // Given
  const DateTime datetime = datetime_make(1582, 11, 1, 0, 0, 0);
  const double expected_jd = 2299177.5;
  const double tolerance = 0.1;

  // When
  double jd = datetime_to_julian(datetime);

  // Then
  TEST_ASSERT_DOUBLE_EQ(jd, expected_jd, tolerance);
}

// Vérifie qu'une date du calendrier julien utilise la branche julienne.
static void test_julian_date_before_gregorian_reform(void) {
  // Given
  const DateTime datetime = datetime_make(1500, 3, 1, 0, 0, 0);
  const double expected_jd = 2268992.5;
  const double tolerance = 0.1;

  // When
  double jd = datetime_to_julian(datetime);

  // Then
  TEST_ASSERT_DOUBLE_EQ(jd, expected_jd, tolerance);
}

// Vérifie que le 29 février 1500 est refusé en calendrier julien.
static void test_datetime_is_valid_accepts_julian_leap_day(void) {
  // Given
  const DateTime datetime = datetime_make(1500, 2, 29, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_TRUE(valid);
}

// Vérifie qu'une date du calendrier grégorien de 1582 est acceptée.
static void test_datetime_is_valid_accepts_gregorian_1582_date(void) {
  // Given
  const DateTime datetime = datetime_make(1582, 10, 15, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_TRUE(valid);
}

// Vérifie qu'une date d'octobre 1582 avant la réforme est acceptée.
static void test_datetime_is_valid_accepts_pre_gregorian_october_date(void) {
  // Given
  const DateTime datetime = datetime_make(1582, 10, 4, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_TRUE(valid);
}

// Vérifie qu'une date antérieure à la réforme grégorienne est acceptée.
static void test_datetime_is_valid_accepts_pre_gregorian_date(void) {
  // Given
  const DateTime datetime = datetime_make(1500, 2, 1, 0, 0, 0);

  // When
  int valid = datetime_is_valid(datetime);

  // Then
  TEST_ASSERT_TRUE(valid);
}

// Vérifie qu'une date julienne utilise la branche julienne du jour de semaine.
static void test_weekday_returns_sunday_for_julian_date(void) {
  // Given
  const DateTime datetime = datetime_make(1500, 3, 1, 0, 0, 0);
  const int expected_weekday = 0;

  // When
  int weekday = datetime_weekday(datetime);

  // Then
  TEST_ASSERT_INT_EQ(weekday, expected_weekday);
}

// Vérifie que le 4 octobre 1582 julien est un jeudi.
static void test_weekday_returns_thursday_before_gregorian_reform(void) {
  // Given
  const DateTime datetime = datetime_make(1582, 10, 4, 0, 0, 0);
  const int expected_weekday = 4;

  // When
  int weekday = datetime_weekday(datetime);

  // Then
  TEST_ASSERT_INT_EQ(weekday, expected_weekday);
}

// Vérifie que le 15 octobre 1582 est un vendredi grégorien.
static void test_weekday_returns_friday_for_gregorian_reform(void) {
  // Given
  const DateTime datetime = datetime_make(1582, 10, 15, 0, 0, 0);
  const int expected_weekday = 5;

  // When
  int weekday = datetime_weekday(datetime);

  // Then
  TEST_ASSERT_INT_EQ(weekday, expected_weekday);
}

// Vérifie que le 1er janvier 1900 est un lundi.
static void test_weekday_returns_monday_for_1900_01_01(void) {
  // Given
  const DateTime datetime = datetime_make(1900, 1, 1, 0, 0, 0);
  const int expected_weekday = 1;

  // When
  int weekday = datetime_weekday(datetime);

  // Then
  TEST_ASSERT_INT_EQ(weekday, expected_weekday);
}

// Vérifie que l'ajout d'un jour conserve l'heure et change la date.
static void test_datetime_add_days_advances_one_day(void) {
  // Given
  const DateTime datetime = datetime_make(2026, 11, 14, 12, 30, 45);
  const DateTime expected = datetime_make(2026, 11, 15, 12, 30, 45);

  // When
  DateTime actual = datetime_add_days(datetime, 1);

  // Then
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
  TEST_ASSERT_INT_EQ(actual.hour, expected.hour);
  TEST_ASSERT_INT_EQ(actual.minute, expected.minute);
  TEST_ASSERT_INT_EQ(actual.second, expected.second);
}

// Vérifie que l'ajout d'un jour franchit une fin de mois.
static void test_datetime_add_days_crosses_month_end(void) {
  // Given
  const DateTime datetime = datetime_make(2026, 11, 30, 0, 0, 0);
  const DateTime expected = datetime_make(2026, 12, 1, 0, 0, 0);

  // When
  DateTime actual = datetime_add_days(datetime, 1);

  // Then
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
}

// Vérifie que l'ajout d'un jour franchit une fin d'année.
static void test_datetime_add_days_crosses_year_end(void) {
  // Given
  const DateTime datetime = datetime_make(2026, 12, 31, 0, 0, 0);
  const DateTime expected = datetime_make(2027, 1, 1, 0, 0, 0);

  // When
  DateTime actual = datetime_add_days(datetime, 1);

  // Then
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
}

// Vérifie que l'ajout d'un nombre négatif de jours recule la date.
static void test_datetime_add_days_subtracts_days(void) {
  // Given
  const DateTime datetime = datetime_make(2026, 3, 1, 0, 0, 0);
  const DateTime expected = datetime_make(2026, 2, 28, 0, 0, 0);

  // When
  DateTime actual = datetime_add_days(datetime, -1);

  // Then
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
}

// Vérifie que le recul d'un jour dans un mois conserve la date.
static void test_datetime_add_days_subtracts_within_month(void) {
  // Given
  const DateTime datetime = datetime_make(2026, 3, 2, 0, 0, 0);
  const DateTime expected = datetime_make(2026, 3, 1, 0, 0, 0);

  // When
  DateTime actual = datetime_add_days(datetime, -1);

  // Then
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
}

// Vérifie que le recul d'un jour franchit la fin d'année.
static void test_datetime_add_days_subtracts_across_year_boundary(void) {
  // Given
  const DateTime datetime = datetime_make(2026, 1, 1, 12, 30, 45);
  const DateTime expected = datetime_make(2025, 12, 31, 12, 30, 45);

  // When
  DateTime actual = datetime_add_days(datetime, -1);

  // Then
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
  TEST_ASSERT_INT_EQ(actual.hour, expected.hour);
  TEST_ASSERT_INT_EQ(actual.minute, expected.minute);
  TEST_ASSERT_INT_EQ(actual.second, expected.second);
}

// Vérifie qu'un mois invalide est renvoyé sans lecture hors limites.
static void test_datetime_add_days_returns_date_with_invalid_month(void) {
  // Given
  const DateTime datetime = datetime_make(2026, 13, 1, 6, 15, 30);
  const DateTime expected = datetime_make(2026, 13, 1, 6, 15, 30);

  // When
  DateTime actual = datetime_add_days(datetime, 5);

  // Then
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
  TEST_ASSERT_INT_EQ(actual.hour, expected.hour);
  TEST_ASSERT_INT_EQ(actual.minute, expected.minute);
  TEST_ASSERT_INT_EQ(actual.second, expected.second);
}

// Vérifie que la conversion grégorienne forcée précède la réforme de 1582.
static void test_datetime_to_julian_gregorian_before_reform(void) {
  // Given
  const DateTime datetime = datetime_make(1500, 3, 1, 0, 0, 0);
  const double expected_jd = 2268982.5;
  const double tolerance = 0.1;

  // When
  const double julian = datetime_to_julian_gregorian(datetime);
  const double automatic = datetime_to_julian(datetime);

  // Then
  TEST_ASSERT_DOUBLE_EQ(julian, expected_jd, tolerance);
  // Le calendrier julien du 1500-03-01 est postérieur de dix jours.
  TEST_ASSERT_DOUBLE_EQ(automatic - julian, 10.0, tolerance);
}

// Vérifie qu'un Julian Day revient à la date et à l'heure d'origine.
static void test_datetime_from_julian_round_trip(void) {
  // Given
  const DateTime expected = datetime_make(2026, 11, 14, 12, 34, 56);
  const double tolerance = 0.0001;

  // When
  const double jd = datetime_to_julian(expected);
  const DateTime actual = datetime_from_julian(jd);

  // Then
  assert_datetime_equals(expected, actual);
  TEST_ASSERT_DOUBLE_EQ(jd, datetime_to_julian(actual), tolerance);
}

// Vérifie qu'un Julian Day de la réforme grégorienne revient au 15 octobre.
static void test_datetime_from_julian_gregorian_reform(void) {
  // Given
  const DateTime expected = datetime_make(1582, 10, 15, 0, 0, 0);

  // When
  const DateTime actual = datetime_from_julian(2299160.5);

  // Then
  assert_datetime_equals(expected, actual);
}

// Vérifie qu'un Julian Day antérieur à la réforme revient au calendrier julien.
static void test_datetime_from_julian_before_reform(void) {
  // Given
  const DateTime expected = datetime_make(1582, 10, 4, 0, 0, 0);

  // When
  const DateTime actual = datetime_from_julian(2299159.5);

  // Then
  assert_datetime_equals(expected, actual);
}

// Vérifie qu'un Julian Day hors plage produit une date invalide.
static void test_datetime_from_julian_rejects_out_of_range(void) {
  // Given
  const double negative = -1.0;
  const double too_large = 8000001.0;
  const double infinite = INFINITY;

  // When
  const DateTime from_negative = datetime_from_julian(negative);
  const DateTime from_large = datetime_from_julian(too_large);
  const DateTime from_infinite = datetime_from_julian(infinite);

  // Then
  TEST_ASSERT_FALSE(datetime_is_valid(from_negative));
  TEST_ASSERT_FALSE(datetime_is_valid(from_large));
  TEST_ASSERT_FALSE(datetime_is_valid(from_infinite));
}

// Vérifie qu'un décalage impossible aux bornes entières laisse la date intacte.
static void test_datetime_add_days_returns_date_for_extreme_shift(void) {
  // Given
  const DateTime datetime = datetime_make(2026, 11, 14, 6, 15, 30);
  const DateTime expected = datetime;

  // When
  const DateTime from_min = datetime_add_days(datetime, INT_MIN);
  const DateTime from_max = datetime_add_days(datetime, INT_MAX);

  // Then
  assert_datetime_equals(expected, from_min);
  assert_datetime_equals(expected, from_max);
}

static const TestCase test_cases[] = {
    {"test_j2000_epoch", test_j2000_epoch},
    {"test_gregorian_date", test_gregorian_date},
    {"test_leap_year", test_leap_year},
    {"test_non_leap_year", test_non_leap_year},
    {"test_weekday_returns_sunday_for_2025_06_15",
     test_weekday_returns_sunday_for_2025_06_15},
    {"test_weekday_returns_saturday_for_2000_01_01",
     test_weekday_returns_saturday_for_2000_01_01},
    {"test_weekday_returns_monday_for_2025_01_06",
     test_weekday_returns_monday_for_2025_01_06},
    {"test_weekday_returns_thursday_for_leap_day_2024_02_29",
     test_weekday_returns_thursday_for_leap_day_2024_02_29},
    {"test_datetime_is_valid_for_valid_date",
     test_datetime_is_valid_for_valid_date},
    {"test_datetime_is_valid_rejects_invalid_month",
     test_datetime_is_valid_rejects_invalid_month},
    {"test_datetime_is_valid_rejects_invalid_day",
     test_datetime_is_valid_rejects_invalid_day},
    {"test_datetime_is_valid_accepts_leap_day",
     test_datetime_is_valid_accepts_leap_day},
    {"test_datetime_is_valid_rejects_non_leap_day",
     test_datetime_is_valid_rejects_non_leap_day},
    {"test_datetime_is_valid_rejects_invalid_time_components",
     test_datetime_is_valid_rejects_invalid_time_components},
    {"test_datetime_is_valid_rejects_invalid_year",
     test_datetime_is_valid_rejects_invalid_year},
    {"test_datetime_is_valid_rejects_year_above_max",
     test_datetime_is_valid_rejects_year_above_max},
    {"test_datetime_is_valid_rejects_month_zero",
     test_datetime_is_valid_rejects_month_zero},
    {"test_datetime_is_valid_rejects_day_zero",
     test_datetime_is_valid_rejects_day_zero},
    {"test_julian_date_before_gregorian_reform",
     test_julian_date_before_gregorian_reform},
    {"test_gregorian_date_in_november_1582",
     test_gregorian_date_in_november_1582},
    {"test_datetime_is_valid_accepts_julian_leap_day",
     test_datetime_is_valid_accepts_julian_leap_day},
    {"test_datetime_is_valid_accepts_gregorian_1582_date",
     test_datetime_is_valid_accepts_gregorian_1582_date},
    {"test_datetime_is_valid_accepts_pre_gregorian_october_date",
     test_datetime_is_valid_accepts_pre_gregorian_october_date},
    {"test_datetime_is_valid_accepts_pre_gregorian_date",
     test_datetime_is_valid_accepts_pre_gregorian_date},
    {"test_weekday_returns_sunday_for_julian_date",
     test_weekday_returns_sunday_for_julian_date},
    {"test_weekday_returns_thursday_before_gregorian_reform",
     test_weekday_returns_thursday_before_gregorian_reform},
    {"test_weekday_returns_friday_for_gregorian_reform",
     test_weekday_returns_friday_for_gregorian_reform},
    {"test_weekday_returns_monday_for_1900_01_01",
     test_weekday_returns_monday_for_1900_01_01},
    {"test_datetime_add_days_advances_one_day",
     test_datetime_add_days_advances_one_day},
    {"test_datetime_add_days_crosses_month_end",
     test_datetime_add_days_crosses_month_end},
    {"test_datetime_add_days_crosses_year_end",
     test_datetime_add_days_crosses_year_end},
    {"test_datetime_add_days_subtracts_days",
     test_datetime_add_days_subtracts_days},
    {"test_datetime_add_days_subtracts_within_month",
     test_datetime_add_days_subtracts_within_month},
    {"test_datetime_add_days_subtracts_across_year_boundary",
     test_datetime_add_days_subtracts_across_year_boundary},
    {"test_datetime_add_days_returns_date_with_invalid_month",
     test_datetime_add_days_returns_date_with_invalid_month},
    {"test_datetime_to_julian_gregorian_before_reform",
     test_datetime_to_julian_gregorian_before_reform},
    {"test_datetime_from_julian_round_trip",
     test_datetime_from_julian_round_trip},
    {"test_datetime_from_julian_gregorian_reform",
     test_datetime_from_julian_gregorian_reform},
    {"test_datetime_from_julian_before_reform",
     test_datetime_from_julian_before_reform},
    {"test_datetime_from_julian_rejects_out_of_range",
     test_datetime_from_julian_rejects_out_of_range},
    {"test_datetime_add_days_returns_date_for_extreme_shift",
     test_datetime_add_days_returns_date_for_extreme_shift},
};

int main(void) {
  return runner_run("datetime", test_cases, TEST_CASE_COUNT(test_cases));
}