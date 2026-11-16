/*
 * test_sun_position.c
 * Tests de la position solaire : plages, déterminisme, équation du centre et
 * référence JPL.
 */

#include <stddef.h>

#include "astro.h"
#include "sun_position.h"
#include "support/test_runner.h"

// Vérifie que la position solaire reste dans les plages physiques attendues.
static void test_sun_position_at_returns_physical_ranges(void) {
  // Given
  const double julian_day = 2461359.0;

  // When
  SunPosition sun;
  sun_position_at(julian_day, &sun);

  // Then
  TEST_ASSERT_TRUE(sun.ecliptic_longitude >= 0.0);
  TEST_ASSERT_TRUE(sun.ecliptic_longitude < 360.0);
  TEST_ASSERT_TRUE(sun.right_ascension >= 0.0);
  TEST_ASSERT_TRUE(sun.right_ascension < 360.0);
  TEST_ASSERT_TRUE(sun.declination >= -90.0);
  TEST_ASSERT_TRUE(sun.declination <= 90.0);
  TEST_ASSERT_TRUE(sun.mean_longitude >= 0.0);
  TEST_ASSERT_TRUE(sun.mean_longitude < 360.0);
  TEST_ASSERT_TRUE(sun.mean_anomaly >= 0.0);
  TEST_ASSERT_TRUE(sun.mean_anomaly < 360.0);
}

// Vérifie que la position est déterministe pour un même Julian Day.
static void test_sun_position_at_is_deterministic(void) {
  // Given
  const double julian_day = 2461359.0;
  SunPosition first;
  SunPosition second;

  // When
  sun_position_at(julian_day, &first);
  sun_position_at(julian_day, &second);

  // Then
  TEST_ASSERT_DOUBLE_EQ(first.ecliptic_longitude, second.ecliptic_longitude,
                        1e-12);
  TEST_ASSERT_DOUBLE_EQ(first.right_ascension, second.right_ascension, 1e-12);
  TEST_ASSERT_DOUBLE_EQ(first.declination, second.declination, 1e-12);
}

// Vérifie que la position solaire est proche des coordonnées apparentes JPL.
// 2026-11-14 19:30 UTC : lambda 232.4417 deg, RA 230.0338 deg,
// Dec -18.3797 deg.
static void test_sun_position_matches_jpl_reference(void) {
  // Given
  const double julian_day = 2461359.3125;
  const double expected_longitude = 232.4417;
  const double expected_right_ascension = 230.0338;
  const double expected_declination = -18.3797;
  const double tolerance = 0.01;

  // When
  SunPosition sun;
  sun_position_at(julian_day, &sun);

  // Then
  TEST_ASSERT_DOUBLE_EQ(sun.ecliptic_longitude, expected_longitude, tolerance);
  TEST_ASSERT_DOUBLE_EQ(sun.right_ascension, expected_right_ascension,
                        tolerance);
  TEST_ASSERT_DOUBLE_EQ(sun.declination, expected_declination, tolerance);
}

// Vérifie que l'équation du centre est appliquée : la longitude vraie s'écarte
// de la longitude moyenne de +1,915 deg à M = 90 deg et de -1,915 deg à
// M = 270 deg.
static void test_sun_position_applies_equation_of_center(void) {
  // Given
  const double julian_days[] = {2451638.8257, 2451821.4555};
  const double expected_equations[] = {1.9145, -1.9145};
  const double tolerance = 0.01;

  // When / Then
  for (size_t i = 0; i < TEST_CASE_COUNT(julian_days); ++i) {
    SunPosition sun;
    sun_position_at(julian_days[i], &sun);

    const double equation =
        astro_norm_180(sun.ecliptic_longitude - sun.mean_longitude);

    TEST_ASSERT_DOUBLE_EQ(equation, expected_equations[i], tolerance);
  }
}

// Vérifie qu'une sortie nulle est ignorée sans écrire.
static void test_sun_position_at_rejects_null_output(void) {
  // Given
  const double julian_day = 2461359.0;

  // When
  sun_position_at(julian_day, NULL);

  // Then
  TEST_ASSERT_TRUE(1);
}

static const TestCase test_cases[] = {
    {"test_sun_position_at_returns_physical_ranges",
     test_sun_position_at_returns_physical_ranges},
    {"test_sun_position_at_is_deterministic",
     test_sun_position_at_is_deterministic},
    {"test_sun_position_matches_jpl_reference",
     test_sun_position_matches_jpl_reference},
    {"test_sun_position_applies_equation_of_center",
     test_sun_position_applies_equation_of_center},
    {"test_sun_position_at_rejects_null_output",
     test_sun_position_at_rejects_null_output},
};

int main(void) {
  return runner_run("sun_position", test_cases, TEST_CASE_COUNT(test_cases));
}