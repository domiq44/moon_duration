/*
 * test_moon_phase.c
 * Tests de la phase lunaire et de ses transitions (nouvelle, quartiers,
 * pleine).
 */

#include <math.h>
#include <stddef.h>

#include "astro.h"
#include "moon_phase.h"
#include "moon_position.h"
#include "sun_position.h"
#include "support/test_runner.h"

// Instants des événements lunaires de janvier 2000, déduits de l'élongation
// Lune-Soleil (voir ALGORITHME.md, section 6). Les classes sont évaluées au
// milieu de leur intervalle pour éviter les frontières.
#define NEW_MOON_2000_01 2451550.255
#define FIRST_QUARTER_2000_01 2451558.045
#define FULL_MOON_2000_01 2451564.655
#define LAST_QUARTER_2000_01 2451571.7598

// Vérifie qu'une nouvelle lune est détectée au JD de référence.
static void test_moon_phase_from_julian_day_new(void) {
  // Given
  const double julian_day = NEW_MOON_2000_01;
  const MoonPhase expected_phase = MOON_NEW;

  // When
  MoonPhase actual_phase = moon_phase_from_julian_day(julian_day);

  // Then
  TEST_ASSERT_INT_EQ((int)actual_phase, (int)expected_phase);
}

// Vérifie qu'une pleine lune est détectée au JD attendu.
static void test_moon_phase_from_julian_day_full(void) {
  // Given
  const double julian_day = FULL_MOON_2000_01;
  const MoonPhase expected_phase = MOON_FULL;

  // When
  MoonPhase actual_phase = moon_phase_from_julian_day(julian_day);

  // Then
  TEST_ASSERT_INT_EQ((int)actual_phase, (int)expected_phase);
}

// Vérifie qu'un premier quartier est détecté au JD attendu.
static void test_moon_phase_from_julian_day_first_quarter(void) {
  // Given
  const double julian_day = FIRST_QUARTER_2000_01;
  const MoonPhase expected_phase = MOON_FIRST_QUARTER;

  // When
  MoonPhase actual_phase = moon_phase_from_julian_day(julian_day);

  // Then
  TEST_ASSERT_INT_EQ((int)actual_phase, (int)expected_phase);
}

// Vérifie qu'un dernier quartier est détecté au JD attendu.
static void test_moon_phase_from_julian_day_last_quarter(void) {
  // Given
  const double julian_day = LAST_QUARTER_2000_01;
  const MoonPhase expected_phase = MOON_LAST_QUARTER;

  // When
  MoonPhase actual_phase = moon_phase_from_julian_day(julian_day);

  // Then
  TEST_ASSERT_INT_EQ((int)actual_phase, (int)expected_phase);
}

// Vérifie qu'un premier croissant est détecté au JD attendu.
static void test_moon_phase_from_julian_day_waxing_crescent(void) {
  // Given
  const double julian_day = 2451554.2888;
  const MoonPhase expected_phase = MOON_WAXING_CRESCENT;

  // When
  MoonPhase actual_phase = moon_phase_from_julian_day(julian_day);

  // Then
  TEST_ASSERT_INT_EQ((int)actual_phase, (int)expected_phase);
}

// Vérifie qu'une lune gibbeuse croissante est détectée au JD attendu.
static void test_moon_phase_from_julian_day_waxing_gibbous(void) {
  // Given
  const double julian_day = 2451561.4388;
  const MoonPhase expected_phase = MOON_WAXING_GIBBOUS;

  // When
  MoonPhase actual_phase = moon_phase_from_julian_day(julian_day);

  // Then
  TEST_ASSERT_INT_EQ((int)actual_phase, (int)expected_phase);
}

// Vérifie qu'une lune gibbeuse décroissante est détectée au JD attendu.
static void test_moon_phase_from_julian_day_waning_gibbous(void) {
  // Given
  const double julian_day = 2451568.0075;
  const MoonPhase expected_phase = MOON_WANING_GIBBOUS;

  // When
  MoonPhase actual_phase = moon_phase_from_julian_day(julian_day);

  // Then
  TEST_ASSERT_INT_EQ((int)actual_phase, (int)expected_phase);
}

// Vérifie qu'un dernier croissant est détecté au JD attendu.
static void test_moon_phase_from_julian_day_waning_crescent(void) {
  // Given
  const double julian_day = 2451575.865;
  const MoonPhase expected_phase = MOON_WANING_CRESCENT;

  // When
  MoonPhase actual_phase = moon_phase_from_julian_day(julian_day);

  // Then
  TEST_ASSERT_INT_EQ((int)actual_phase, (int)expected_phase);
}

// Vérifie qu'un cycle lunaire complet énumère les huit phases dans l'ordre.
static void test_moon_phase_cycles_through_every_phase_in_order(void) {
  // Given
  const MoonPhase expected_sequence[] = {
      MOON_NEW,  MOON_WAXING_CRESCENT, MOON_FIRST_QUARTER, MOON_WAXING_GIBBOUS,
      MOON_FULL, MOON_WANING_GIBBOUS,  MOON_LAST_QUARTER,  MOON_WANING_CRESCENT,
      MOON_NEW};
  const double start_julian_day = 2451550.0;
  const int step_count = 4000;

  // When
  MoonPhase sequence[16];
  int length = 0;
  for (int i = 0; i < step_count && length < 16; ++i) {
    const double julian_day = start_julian_day + 30.0 * i / step_count;
    const MoonPhase phase = moon_phase_from_julian_day(julian_day);

    if (length == 0 || sequence[length - 1] != phase) {
      sequence[length++] = phase;
    }
  }

  // Then
  TEST_ASSERT_INT_EQ(length, (int)TEST_CASE_COUNT(expected_sequence));
  for (int i = 0; i < length; ++i) {
    if (sequence[i] != expected_sequence[i]) {
      runner_context_fail(__FILE__, __LINE__,
                          "phase %d : attendu %d, obtenu %d", i,
                          (int)expected_sequence[i], (int)sequence[i]);
      return;
    }
  }
}

// Vérifie que l'illumination est proche de zéro à la nouvelle lune.
static void test_moon_illumination_is_zero_at_new_moon(void) {
  // Given
  const double julian_day = NEW_MOON_2000_01;
  const double expected_illumination = 0.0;
  const double tolerance = 0.01;

  // When
  double actual_illumination = moon_illumination(julian_day);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual_illumination, expected_illumination, tolerance);
}

// Vérifie que l'illumination est proche de un à la pleine lune.
static void test_moon_illumination_is_one_at_full_moon(void) {
  // Given
  const double julian_day = FULL_MOON_2000_01;
  const double expected_illumination = 1.0;
  const double tolerance = 0.01;

  // When
  double actual_illumination = moon_illumination(julian_day);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual_illumination, expected_illumination, tolerance);
}

// Vérifie que l'illumination est proche de un demi au premier quartier.
static void test_moon_illumination_is_half_at_first_quarter(void) {
  // Given
  const double julian_day = FIRST_QUARTER_2000_01;
  const double expected_illumination = 0.5;
  const double tolerance = 0.01;

  // When
  double actual_illumination = moon_illumination(julian_day);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual_illumination, expected_illumination, tolerance);
}

// Vérifie que l'illumination est proche de un demi au dernier quartier.
static void test_moon_illumination_is_half_at_last_quarter(void) {
  // Given
  const double julian_day = LAST_QUARTER_2000_01;
  const double expected_illumination = 0.5;
  const double tolerance = 0.01;

  // When
  double actual_illumination = moon_illumination(julian_day);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual_illumination, expected_illumination, tolerance);
}

// Vérifie que l'illumination suit l'élongation géométrique Lune-Soleil, et non
// la période synodique moyenne.
static void test_moon_illumination_matches_geometric_elongation(void) {
  // Given
  const double julian_days[] = {2451545.0, 2461353.5, 2461358.9583,
                                2461369.0, 2461375.5, 2461380.0};
  const double tolerance = 1e-12;

  // When / Then
  for (size_t i = 0; i < TEST_CASE_COUNT(julian_days); ++i) {
    MoonPosition moon;
    SunPosition sun;
    moon_position_at(julian_days[i], &moon);
    sun_position_at(julian_days[i], &sun);

    const double difference =
        astro_norm_360(moon.ecliptic_longitude - sun.ecliptic_longitude);
    const double cosine =
        astro_cosd(moon.ecliptic_latitude) * astro_cosd(difference);
    const double expected = (1.0 - cosine) / 2.0;

    TEST_ASSERT_DOUBLE_EQ(moon_illumination(julian_days[i]), expected,
                          tolerance);
  }
}

// Vérifie que l'illumination ne retombe pas sur la période synodique : au
// 11 novembre 2025, l'élongation vraie donne 0,524 alors que la période
// synodique moyenne donnait 0,606.
static void test_moon_illumination_uses_true_elongation(void) {
  // Given
  const double julian_day = 2460991.5;
  const double expected_illumination = 0.5239;
  const double tolerance = 0.005;

  // When
  const double actual_illumination = moon_illumination(julian_day);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual_illumination, expected_illumination, tolerance);
}

// Vérifie que l'illumination reste dans [0, 1] sur huit années.
static void test_moon_illumination_stays_within_bounds(void) {
  // Given
  const double start_julian_day = 2460000.5;
  const int day_count = 365 * 8;

  // When / Then
  for (int i = 0; i < day_count; ++i) {
    const double illumination = moon_illumination(start_julian_day + (double)i);

    TEST_ASSERT_TRUE(isfinite(illumination));
    TEST_ASSERT_TRUE(illumination >= 0.0);
    TEST_ASSERT_TRUE(illumination <= 1.0);
  }
}

// Vérifie qu'une date antérieure à l'époque de référence reste une nouvelle
// lune.
static void test_moon_phase_from_julian_day_before_reference_epoch(void) {
  // Given
  const double julian_day = NEW_MOON_2000_01 - 0.3;
  const MoonPhase expected_phase = MOON_NEW;

  // When
  MoonPhase actual_phase = moon_phase_from_julian_day(julian_day);

  // Then
  TEST_ASSERT_INT_EQ((int)actual_phase, (int)expected_phase);
}

// Vérifie qu'un cycle négatif complet est ramené à la phase de référence.
static void test_moon_phase_from_julian_day_negative_synodic_cycle(void) {
  // Given
  const double julian_day = NEW_MOON_2000_01 - 29.530588853;
  const MoonPhase expected_phase = MOON_NEW;

  // When
  MoonPhase actual_phase = moon_phase_from_julian_day(julian_day);

  // Then
  TEST_ASSERT_INT_EQ((int)actual_phase, (int)expected_phase);
}

// Vérifie qu'un Julian Day non fini produit une phase explicitement invalide,
// plutôt qu'une phase quelconque choisie par défaut.
static void test_moon_phase_from_julian_day_rejects_non_finite(void) {
  // Given
  const double non_finite_days[] = {NAN, INFINITY, -INFINITY};
  const int case_count =
      (int)(sizeof(non_finite_days) / sizeof(non_finite_days[0]));
  const MoonPhase expected_phase = MOON_PHASE_INVALID;

  // When / Then
  for (int i = 0; i < case_count; ++i) {
    const MoonPhase actual_phase =
        moon_phase_from_julian_day(non_finite_days[i]);

    TEST_ASSERT_INT_EQ((int)actual_phase, (int)expected_phase);
  }
}

// Vérifie qu'un Julian Day non fini rend une illumination indéterminée.
static void test_moon_illumination_rejects_non_finite(void) {
  // Given
  const double non_finite_days[] = {NAN, INFINITY, -INFINITY};

  // When / Then
  for (size_t i = 0; i < TEST_CASE_COUNT(non_finite_days); ++i) {
    const double actual_illumination = moon_illumination(non_finite_days[i]);

    TEST_ASSERT_TRUE(isnan(actual_illumination));
  }
}

// Vérifie que l'illumination retombe sur la période synodique lorsque la
// position lunaire n'est pas exploitable (Julian Day débordant).
static void test_moon_illumination_uses_synodic_fallback_for_unusable_position(
    void) {
  // Given
  const double julian_day = 1e308;

  // When
  const double actual_illumination = moon_illumination(julian_day);

  // Then
  TEST_ASSERT_TRUE(isfinite(actual_illumination));
  TEST_ASSERT_TRUE(actual_illumination >= 0.0);
  TEST_ASSERT_TRUE(actual_illumination <= 1.0);
}

// Vérifie que la phase retombe sur la période synodique lorsque la position
// lunaire n'est pas exploitable (Julian Day débordant et négatif).
static void test_moon_phase_uses_synodic_fallback_for_unusable_position(void) {
  // Given
  const double julian_day = -1e308;

  // When
  const MoonPhase actual_phase = moon_phase_from_julian_day(julian_day);

  // Then
  TEST_ASSERT_TRUE(actual_phase != MOON_PHASE_INVALID);
  TEST_ASSERT_TRUE((int)actual_phase >= (int)MOON_NEW);
  TEST_ASSERT_TRUE((int)actual_phase <= (int)MOON_WANING_CRESCENT);
}

static const TestCase test_cases[] = {
    {"test_moon_phase_from_julian_day_new",
     test_moon_phase_from_julian_day_new},
    {"test_moon_phase_from_julian_day_full",
     test_moon_phase_from_julian_day_full},
    {"test_moon_phase_from_julian_day_first_quarter",
     test_moon_phase_from_julian_day_first_quarter},
    {"test_moon_phase_from_julian_day_last_quarter",
     test_moon_phase_from_julian_day_last_quarter},
    {"test_moon_phase_from_julian_day_waxing_crescent",
     test_moon_phase_from_julian_day_waxing_crescent},
    {"test_moon_phase_from_julian_day_waxing_gibbous",
     test_moon_phase_from_julian_day_waxing_gibbous},
    {"test_moon_phase_from_julian_day_waning_gibbous",
     test_moon_phase_from_julian_day_waning_gibbous},
    {"test_moon_phase_from_julian_day_waning_crescent",
     test_moon_phase_from_julian_day_waning_crescent},
    {"test_moon_phase_cycles_through_every_phase_in_order",
     test_moon_phase_cycles_through_every_phase_in_order},
    {"test_moon_illumination_is_zero_at_new_moon",
     test_moon_illumination_is_zero_at_new_moon},
    {"test_moon_illumination_is_one_at_full_moon",
     test_moon_illumination_is_one_at_full_moon},
    {"test_moon_illumination_is_half_at_first_quarter",
     test_moon_illumination_is_half_at_first_quarter},
    {"test_moon_illumination_is_half_at_last_quarter",
     test_moon_illumination_is_half_at_last_quarter},
    {"test_moon_illumination_matches_geometric_elongation",
     test_moon_illumination_matches_geometric_elongation},
    {"test_moon_illumination_uses_true_elongation",
     test_moon_illumination_uses_true_elongation},
    {"test_moon_illumination_stays_within_bounds",
     test_moon_illumination_stays_within_bounds},
    {"test_moon_phase_from_julian_day_before_reference_epoch",
     test_moon_phase_from_julian_day_before_reference_epoch},
    {"test_moon_phase_from_julian_day_negative_synodic_cycle",
     test_moon_phase_from_julian_day_negative_synodic_cycle},
    {"test_moon_phase_from_julian_day_rejects_non_finite",
     test_moon_phase_from_julian_day_rejects_non_finite},
    {"test_moon_illumination_rejects_non_finite",
     test_moon_illumination_rejects_non_finite},
    {"test_moon_illumination_uses_synodic_fallback_for_unusable_position",
     test_moon_illumination_uses_synodic_fallback_for_unusable_position},
    {"test_moon_phase_uses_synodic_fallback_for_unusable_position",
     test_moon_phase_uses_synodic_fallback_for_unusable_position},
};

int main(void) {
  return runner_run("moon_phase", test_cases, TEST_CASE_COUNT(test_cases));
}