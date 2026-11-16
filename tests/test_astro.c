/*
 * test_astro.c
 * Tests des conversions et constantes astronomiques : normalisation d'angles,
 * trigonométrie et clamp de `asind`.
 */

#include <stddef.h>

#include "astro.h"
#include "support/test_runner.h"

// Vérifie que la normalisation ramène tout angle dans [0, 360).
static void test_astro_norm_360_wraps_angles_into_range(void) {
  // Given
  const double angles[] = {370.0, -10.0, 360.0, -360.0, 725.0};
  const double expected[] = {10.0, 350.0, 0.0, 0.0, 5.0};
  const double tolerance = 1e-9;

  // When / Then
  for (size_t i = 0; i < TEST_CASE_COUNT(angles); ++i) {
    TEST_ASSERT_DOUBLE_EQ(astro_norm_360(angles[i]), expected[i], tolerance);
  }
}

// Vérifie que la normalisation ramène tout angle dans [-180, 180].
static void test_astro_norm_180_wraps_angles_into_range(void) {
  // Given
  const double angles[] = {190.0, -190.0, 550.0, -550.0};
  const double expected[] = {-170.0, 170.0, -170.0, 170.0};
  const double tolerance = 1e-9;

  // When / Then
  for (size_t i = 0; i < TEST_CASE_COUNT(angles); ++i) {
    TEST_ASSERT_DOUBLE_EQ(astro_norm_180(angles[i]), expected[i], tolerance);
  }
}

// Vérifie la trigonométrie en degrés sur des angles remarquables.
static void test_astro_trigonometry_matches_known_angles(void) {
  // Given
  const double tolerance = 1e-9;

  // When / Then
  TEST_ASSERT_DOUBLE_EQ(astro_sind(30.0), 0.5, tolerance);
  TEST_ASSERT_DOUBLE_EQ(astro_cosd(60.0), 0.5, tolerance);
  TEST_ASSERT_DOUBLE_EQ(astro_sind(90.0), 1.0, tolerance);
  TEST_ASSERT_DOUBLE_EQ(astro_cosd(180.0), -1.0, tolerance);
  TEST_ASSERT_DOUBLE_EQ(astro_atan2d(1.0, 1.0), 45.0, tolerance);
  TEST_ASSERT_DOUBLE_EQ(astro_atan2d(0.0, -1.0), 180.0, tolerance);
}

// Vérifie que l'arc sinus borne ses entrées à l'intervalle [-1, 1].
static void test_astro_asind_clamps_out_of_range_values(void) {
  // Given
  const double tolerance = 1e-9;

  // When / Then
  TEST_ASSERT_DOUBLE_EQ(astro_asind(2.0), 90.0, tolerance);
  TEST_ASSERT_DOUBLE_EQ(astro_asind(-2.0), -90.0, tolerance);
  TEST_ASSERT_DOUBLE_EQ(astro_asind(0.5), 30.0, tolerance);
}

// Vérifie la conversion écliptique vers équatoriale sur des points cardinaux.
static void test_astro_ecliptic_to_equatorial_converts_cardinal_points(void) {
  // Given
  const double julian_day = 2451545.0;
  const double tolerance = 1e-6;
  double right_ascension;
  double declination;

  // When
  astro_ecliptic_to_equatorial(0.0, 0.0, julian_day, &right_ascension,
                               &declination);

  // Then
  TEST_ASSERT_DOUBLE_EQ(right_ascension, 0.0, tolerance);
  TEST_ASSERT_DOUBLE_EQ(declination, 0.0, tolerance);

  // When
  astro_ecliptic_to_equatorial(90.0, 0.0, julian_day, &right_ascension,
                               &declination);

  // Then
  // Le point à 90° de longitude écliptique culmine à la déclinaison de
  // l'obliquité (environ 23,44°).
  TEST_ASSERT_DOUBLE_EQ(right_ascension, 90.0, tolerance);
  TEST_ASSERT_DOUBLE_EQ(declination, 23.4393, 1e-3);
}

// Vérifie que les sorties optionnelles peuvent être omises.
static void test_astro_ecliptic_to_equatorial_accepts_null_outputs(void) {
  // Given
  const double julian_day = 2451545.0;

  // When
  astro_ecliptic_to_equatorial(0.0, 0.0, julian_day, NULL, NULL);

  // Then
  TEST_ASSERT_TRUE(1);
}

// Vérifie que le temps sidéral reste dans [0, 360) et reste déterministe.
static void test_astro_sidereal_mean_degrees_is_bounded(void) {
  // Given
  const double julian_day = 2461359.0;

  // When
  const double first = astro_sidereal_mean_degrees(julian_day);
  const double second = astro_sidereal_mean_degrees(julian_day);

  // Then
  TEST_ASSERT_TRUE(first >= 0.0);
  TEST_ASSERT_TRUE(first < 360.0);
  TEST_ASSERT_DOUBLE_EQ(first, second, 1e-12);
}

static const TestCase test_cases[] = {
    {"test_astro_norm_360_wraps_angles_into_range",
     test_astro_norm_360_wraps_angles_into_range},
    {"test_astro_norm_180_wraps_angles_into_range",
     test_astro_norm_180_wraps_angles_into_range},
    {"test_astro_trigonometry_matches_known_angles",
     test_astro_trigonometry_matches_known_angles},
    {"test_astro_asind_clamps_out_of_range_values",
     test_astro_asind_clamps_out_of_range_values},
    {"test_astro_ecliptic_to_equatorial_converts_cardinal_points",
     test_astro_ecliptic_to_equatorial_converts_cardinal_points},
    {"test_astro_ecliptic_to_equatorial_accepts_null_outputs",
     test_astro_ecliptic_to_equatorial_accepts_null_outputs},
    {"test_astro_sidereal_mean_degrees_is_bounded",
     test_astro_sidereal_mean_degrees_is_bounded},
};

int main(void) {
  return runner_run("astro", test_cases, TEST_CASE_COUNT(test_cases));
}