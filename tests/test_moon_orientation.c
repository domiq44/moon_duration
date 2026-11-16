/*
 * test_moon_orientation.c
 * Tests de l'orientation du limbe brillant : plage, déterminisme et entrées
 * invalides.
 */

#include <math.h>
#include <stddef.h>

#include "astro.h"
#include "moon_orientation.h"
#include "moon_position.h"
#include "sun_position.h"
#include "support/test_runner.h"

// Convertit une ascension droite et une déclinaison en vecteur unitaire dans le
// repère local (est, nord, zénith).
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static void local_unit_vector(double right_ascension, double declination,
                              double latitude, double longitude,
                              double julian_day, double *vector) {
  const double local_sidereal =
      astro_norm_360(astro_sidereal_mean_degrees(julian_day) + longitude);
  const double hour_angle = astro_norm_180(local_sidereal - right_ascension);

  vector[0] = -astro_cosd(declination) * astro_sind(hour_angle);
  vector[1] =
      astro_sind(declination) * astro_cosd(latitude) -
      astro_cosd(declination) * astro_sind(latitude) * astro_cosd(hour_angle);
  vector[2] =
      astro_sind(declination) * astro_sind(latitude) +
      astro_cosd(declination) * astro_cosd(latitude) * astro_cosd(hour_angle);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// Angle du limbe par la méthode vectorielle : le zénith et le Soleil sont
// projetés sur le plan tangent à la Lune, puis l'angle entre les deux
// projections est mesuré depuis le zénith vers l'est.
// Ce calcul est indépendant de la formule trigonométrique chi - q.
static double bright_limb_angle_vector(double julian_day, double latitude,
                                       double longitude) {
  MoonPosition moon;
  SunPosition sun;
  moon_position_at(julian_day, &moon);
  sun_position_at(julian_day, &sun);

  double moon_vector[3];
  double sun_vector[3];
  local_unit_vector(moon.right_ascension, moon.declination, latitude, longitude,
                    julian_day, moon_vector);
  local_unit_vector(sun.right_ascension, sun.declination, latitude, longitude,
                    julian_day, sun_vector);

  const double sun_dot_moon = sun_vector[0] * moon_vector[0] +
                              sun_vector[1] * moon_vector[1] +
                              sun_vector[2] * moon_vector[2];

  const double zenith[3] = {-moon_vector[0] * moon_vector[2],
                            -moon_vector[1] * moon_vector[2],
                            1.0 - moon_vector[2] * moon_vector[2]};
  const double bright_limb[3] = {sun_vector[0] - sun_dot_moon * moon_vector[0],
                                 sun_vector[1] - sun_dot_moon * moon_vector[1],
                                 sun_vector[2] - sun_dot_moon * moon_vector[2]};

  const double cross[3] = {
      bright_limb[1] * zenith[2] - bright_limb[2] * zenith[1],
      bright_limb[2] * zenith[0] - bright_limb[0] * zenith[2],
      bright_limb[0] * zenith[1] - bright_limb[1] * zenith[0]};

  const double y = cross[0] * moon_vector[0] + cross[1] * moon_vector[1] +
                   cross[2] * moon_vector[2];
  const double x = zenith[0] * bright_limb[0] + zenith[1] * bright_limb[1] +
                   zenith[2] * bright_limb[2];

  return astro_norm_360(astro_atan2d(y, x));
}

// Vérifie que l'angle géocentrique reste dans [0, 360).
static void test_bright_limb_angle_geocentric_is_in_range(void) {
  // Given
  const double julian_day = 2461359.0;

  // When
  const double angle = moon_bright_limb_angle_geocentric(julian_day);

  // Then
  TEST_ASSERT_TRUE(isfinite(angle));
  TEST_ASSERT_TRUE(angle >= 0.0);
  TEST_ASSERT_TRUE(angle < 360.0);
}

// Vérifie que l'angle apparent reste dans [0, 360) sous plusieurs latitudes.
static void test_bright_limb_angle_is_in_range_for_every_latitude(void) {
  // Given
  const double julian_day = 2461359.0;
  const double latitudes[] = {-90.0, -33.8688, 0.0, 48.8566, 90.0};
  const double longitude = 2.3522;

  // When / Then
  for (size_t i = 0; i < TEST_CASE_COUNT(latitudes); i++) {
    const double angle =
        moon_bright_limb_angle(julian_day, latitudes[i], longitude);

    TEST_ASSERT_TRUE(isfinite(angle));
    TEST_ASSERT_TRUE(angle >= 0.0);
    TEST_ASSERT_TRUE(angle < 360.0);
  }
}

// Vérifie que l'angle apparent est déterministe pour un même instant.
static void test_bright_limb_angle_is_deterministic(void) {
  // Given
  const double julian_day = 2461359.0;
  const double latitude = 48.8566;
  const double longitude = 2.3522;

  // When
  const double first = moon_bright_limb_angle(julian_day, latitude, longitude);
  const double second = moon_bright_limb_angle(julian_day, latitude, longitude);

  // Then
  TEST_ASSERT_DOUBLE_EQ(first, second, 1e-12);
}

// Vérifie qu'un Julian Day non fini rend l'orientation indéterminée.
static void test_orientation_rejects_non_finite_julian_day(void) {
  // Given
  const double invalid_julian_day = NAN;

  // When / Then
  TEST_ASSERT_TRUE(
      isnan(moon_bright_limb_angle_geocentric(invalid_julian_day)));
  TEST_ASSERT_TRUE(
      isnan(moon_bright_limb_angle(invalid_julian_day, 48.8566, 2.3522)));
  TEST_ASSERT_TRUE(
      isnan(moon_parallactic_angle(invalid_julian_day, 48.8566, 2.3522)));
}

// Vérifie qu'une position non finie rend l'angle apparent indéterminé.
static void test_bright_limb_angle_rejects_non_finite_position(void) {
  // Given
  const double julian_day = 2461359.0;

  // When / Then
  TEST_ASSERT_TRUE(isnan(moon_bright_limb_angle(julian_day, NAN, 2.3522)));
  TEST_ASSERT_TRUE(isnan(moon_bright_limb_angle(julian_day, 48.8566, NAN)));
  TEST_ASSERT_TRUE(isnan(moon_parallactic_angle(julian_day, NAN, 2.3522)));
}

// Vérifie que le limbe n'est pas identifiable à la nouvelle lune ni à la pleine
// lune, et qu'il l'est au premier quartier.
static void test_bright_limb_angle_is_undefined_at_new_and_full_moon(void) {
  // Given
  const double new_moon_julian_day = 2461383.8;
  const double first_quarter_julian_day = 2461376.4;
  const double full_moon_julian_day = 2461369.0;

  // When / Then
  TEST_ASSERT_TRUE(
      isnan(moon_bright_limb_angle_geocentric(new_moon_julian_day)));
  TEST_ASSERT_TRUE(
      isnan(moon_bright_limb_angle(new_moon_julian_day, 48.8566, 2.3522)));
  TEST_ASSERT_TRUE(
      isfinite(moon_bright_limb_angle_geocentric(first_quarter_julian_day)));
  TEST_ASSERT_TRUE(
      isnan(moon_bright_limb_angle_geocentric(full_moon_julian_day)));
  TEST_ASSERT_TRUE(
      isnan(moon_bright_limb_angle(full_moon_julian_day, 48.8566, 2.3522)));
}

// Vérifie que l'angle apparent est conforme à la méthode vectorielle
// indépendante, sous plusieurs latitudes.
static void test_bright_limb_angle_matches_vector_method(void) {
  // Given
  const double julian_day = 2461359.0;
  const double latitudes[] = {-33.8688, 0.0, 48.8566, 64.1466};
  const double longitude = 2.3522;
  const double tolerance = 1e-9;

  // When / Then
  for (size_t i = 0; i < TEST_CASE_COUNT(latitudes); i++) {
    const double expected =
        bright_limb_angle_vector(julian_day, latitudes[i], longitude);
    const double actual =
        moon_bright_limb_angle(julian_day, latitudes[i], longitude);

    TEST_ASSERT_DOUBLE_EQ(actual, expected, tolerance);
  }
}

// Vérifie que l'angle apparent correspond à la référence de la suite
// moon_position : 2026-11-14 12:00 UTC, Paris.
static void test_bright_limb_angle_matches_reference(void) {
  // Given
  const double julian_day = 2461359.0;
  const double latitude = 48.8566;
  const double longitude = 2.3522;
  const double expected_geocentric = 263.48;
  const double expected_apparent = 296.36;
  const double tolerance = 0.05;

  // When
  const double geocentric = moon_bright_limb_angle_geocentric(julian_day);
  const double apparent =
      moon_bright_limb_angle(julian_day, latitude, longitude);

  // Then
  TEST_ASSERT_DOUBLE_EQ(geocentric, expected_geocentric, tolerance);
  TEST_ASSERT_DOUBLE_EQ(apparent, expected_apparent, tolerance);
}

// Vérifie que l'angle parallactique reste dans [-180, 180] et qu'il est
// défini aux pôles, où la tangente de la latitude diverge.
static void test_parallactic_angle_is_bounded_and_finite_at_poles(void) {
  // Given
  const double julian_day = 2461359.0;
  const double latitudes[] = {-90.0, 0.0, 90.0};
  const double longitude = 2.3522;

  // When / Then
  for (size_t i = 0; i < TEST_CASE_COUNT(latitudes); i++) {
    const double angle =
        moon_parallactic_angle(julian_day, latitudes[i], longitude);

    TEST_ASSERT_TRUE(isfinite(angle));
    TEST_ASSERT_TRUE(angle >= -180.0);
    TEST_ASSERT_TRUE(angle <= 180.0);
  }
}

// Vérifie que l'angle parallactique s'annule lorsque la Lune traverse le
// méridien de l'observateur.
static void test_parallactic_angle_is_zero_on_meridian(void) {
  // Given
  const double julian_day = 2461359.0;
  const double latitude = 48.8566;
  MoonPosition moon;
  moon_position_at(julian_day, &moon);

  // La longitude pour laquelle la Lune est exactement sur le méridien.
  const double longitude = astro_norm_180(
      moon.right_ascension - astro_sidereal_mean_degrees(julian_day));

  // When
  const double angle = moon_parallactic_angle(julian_day, latitude, longitude);

  // Then
  TEST_ASSERT_DOUBLE_EQ(angle, 0.0, 1e-9);
}

static const TestCase test_cases[] = {
    {"test_bright_limb_angle_geocentric_is_in_range",
     test_bright_limb_angle_geocentric_is_in_range},
    {"test_bright_limb_angle_is_in_range_for_every_latitude",
     test_bright_limb_angle_is_in_range_for_every_latitude},
    {"test_bright_limb_angle_is_deterministic",
     test_bright_limb_angle_is_deterministic},
    {"test_orientation_rejects_non_finite_julian_day",
     test_orientation_rejects_non_finite_julian_day},
    {"test_bright_limb_angle_rejects_non_finite_position",
     test_bright_limb_angle_rejects_non_finite_position},
    {"test_bright_limb_angle_is_undefined_at_new_and_full_moon",
     test_bright_limb_angle_is_undefined_at_new_and_full_moon},
    {"test_bright_limb_angle_matches_vector_method",
     test_bright_limb_angle_matches_vector_method},
    {"test_bright_limb_angle_matches_reference",
     test_bright_limb_angle_matches_reference},
    {"test_parallactic_angle_is_bounded_and_finite_at_poles",
     test_parallactic_angle_is_bounded_and_finite_at_poles},
    {"test_parallactic_angle_is_zero_on_meridian",
     test_parallactic_angle_is_zero_on_meridian},
};

int main(void) {
  return runner_run("moon_orientation", test_cases,
                    TEST_CASE_COUNT(test_cases));
}