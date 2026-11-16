/*
 * test_moon_position.c
 * Tests de la position lunaire : plages physiques, déterminisme et référence
 * JPL.
 */

#include "moon_position.h"
#include "support/test_runner.h"

// Vérifie que la position lunaire est dans les plages physiques attendues.
static void test_moon_position_at_returns_physical_ranges(void) {
  // Given
  const double julian_day = 2461359.0;

  // When
  MoonPosition position;
  moon_position_at(julian_day, &position);

  // Then
  TEST_ASSERT_TRUE(position.right_ascension >= 0.0);
  TEST_ASSERT_TRUE(position.right_ascension < 360.0);
  TEST_ASSERT_TRUE(position.declination >= -90.0);
  TEST_ASSERT_TRUE(position.declination <= 90.0);
  TEST_ASSERT_TRUE(position.ecliptic_longitude >= 0.0);
  TEST_ASSERT_TRUE(position.ecliptic_longitude < 360.0);
  TEST_ASSERT_TRUE(position.ecliptic_latitude >= -90.0);
  TEST_ASSERT_TRUE(position.ecliptic_latitude <= 90.0);
}

// Vérifie que la distance lunaire reste dans l'intervalle perigee-apogee.
static void test_moon_position_distance_is_within_expected_bounds(void) {
  // Given
  const double julian_day = 2461359.0;
  const double minimum_km = 356000.0;
  const double maximum_km = 407000.0;

  // When
  MoonPosition position;
  moon_position_at(julian_day, &position);

  // Then
  TEST_ASSERT_TRUE(position.distance_km >= minimum_km);
  TEST_ASSERT_TRUE(position.distance_km <= maximum_km);
}

// Vérifie que la position est déterministe pour un même Julian Day.
static void test_moon_position_at_is_deterministic(void) {
  // Given
  const double julian_day = 2461359.0;
  MoonPosition first;
  MoonPosition second;

  // When
  moon_position_at(julian_day, &first);
  moon_position_at(julian_day, &second);

  // Then
  TEST_ASSERT_DOUBLE_EQ(first.right_ascension, second.right_ascension, 1e-12);
  TEST_ASSERT_DOUBLE_EQ(first.declination, second.declination, 1e-12);
  TEST_ASSERT_DOUBLE_EQ(first.distance_km, second.distance_km, 1e-9);
}

// Vérifie que la position est proche des valeurs JPL de référence.
// 2026-11-14 12:00 UTC : RA 291.13 deg, Dec -24.99 deg, distance 405343 km.
static void test_moon_position_matches_jpl_reference(void) {
  // Given
  const double julian_day = 2461359.0;
  const double expected_ra = 291.13;
  const double expected_declination = -24.99;
  const double expected_distance_km = 405343.0;
  const double angle_tolerance = 0.5;
  const double distance_tolerance = 1000.0;

  // When
  MoonPosition position;
  moon_position_at(julian_day, &position);

  // Then
  TEST_ASSERT_DOUBLE_EQ(position.right_ascension, expected_ra, angle_tolerance);
  TEST_ASSERT_DOUBLE_EQ(position.declination, expected_declination,
                        angle_tolerance);
  TEST_ASSERT_DOUBLE_EQ(position.distance_km, expected_distance_km,
                        distance_tolerance);
}

// Vérifie que l'altitude reste dans l'intervalle [-90, 90].
static void test_moon_altitude_is_bounded(void) {
  // Given
  const double julian_day = 2461359.0;
  const double latitude = 48.8566;
  const double longitude = 2.3522;

  // When
  double altitude = moon_altitude(julian_day, latitude, longitude);

  // Then
  TEST_ASSERT_TRUE(altitude >= -90.0);
  TEST_ASSERT_TRUE(altitude <= 90.0);
}

// Vérifie qu'une sortie nulle est ignorée sans écrire.
static void test_moon_position_at_rejects_null_output(void) {
  // Given
  const double julian_day = 2461359.0;

  // When
  moon_position_at(julian_day, NULL);

  // Then
  TEST_ASSERT_TRUE(1);
}

static const TestCase test_cases[] = {
    {"test_moon_position_at_returns_physical_ranges",
     test_moon_position_at_returns_physical_ranges},
    {"test_moon_position_distance_is_within_expected_bounds",
     test_moon_position_distance_is_within_expected_bounds},
    {"test_moon_position_at_is_deterministic",
     test_moon_position_at_is_deterministic},
    {"test_moon_position_matches_jpl_reference",
     test_moon_position_matches_jpl_reference},
    {"test_moon_altitude_is_bounded", test_moon_altitude_is_bounded},
    {"test_moon_position_at_rejects_null_output",
     test_moon_position_at_rejects_null_output},
};

int main(void) {
  return runner_run("moon_position", test_cases, TEST_CASE_COUNT(test_cases));
}