/*
 * test_moon_events.c
 * Tests de la détection des événements : alternance, voisinage de l'horizon et
 * entrées invalides.
 */

#include "moon_events.h"
#include "moon_position.h"
#include "support/test_runner.h"

#include <math.h>

#define PARIS_LATITUDE 48.8566
#define PARIS_LONGITUDE 2.3522
#define EVENT_CAPACITY 32

// Vérifie que le scan trouve des événements alternés pour Paris.
static void test_moon_events_scan_finds_alternating_events(void) {
  // Given
  const double start_jd = 2461358.5 - 1.0 / 24.0;
  const double end_jd = start_jd + 3.0;
  MoonEvent events[EVENT_CAPACITY];

  // When
  int count = moon_events_scan(start_jd, end_jd, PARIS_LATITUDE,
                               PARIS_LONGITUDE, events, EVENT_CAPACITY);

  // Then
  TEST_ASSERT_TRUE(count >= 4);
  for (int i = 0; i < count; i++) {
    const MoonEventType expected =
        (i % 2 == 0) ? MOON_EVENT_RISE : MOON_EVENT_SET;
    TEST_ASSERT_INT_EQ((int)events[i].type, (int)expected);
    if (i > 0) {
      TEST_ASSERT_TRUE(events[i].julian_day > events[i - 1].julian_day);
    }
  }
}

// Vérifie que chaque événement est proche de l'horizon.
static void test_moon_events_scan_events_are_near_horizon(void) {
  // Given
  const double start_jd = 2461358.5 - 1.0 / 24.0;
  const double end_jd = start_jd + 2.0;
  MoonEvent events[EVENT_CAPACITY];
  const double tolerance = 1e-4;

  // When
  int count = moon_events_scan(start_jd, end_jd, PARIS_LATITUDE,
                               PARIS_LONGITUDE, events, EVENT_CAPACITY);

  // Then
  TEST_ASSERT_TRUE(count > 0);
  for (int i = 0; i < count; i++) {
    const double altitude =
        moon_altitude(events[i].julian_day, PARIS_LATITUDE, PARIS_LONGITUDE);
    TEST_ASSERT_DOUBLE_EQ(altitude, MOON_HORIZON_ALTITUDE_DEG, tolerance);
  }
}

// Vérifie que le scan refuse une sortie nulle.
static void test_moon_events_scan_rejects_null_output(void) {
  // Given
  const double start_jd = 2461358.5;
  const double end_jd = start_jd + 1.0;

  // When
  int status = moon_events_scan(start_jd, end_jd, PARIS_LATITUDE,
                                PARIS_LONGITUDE, NULL, EVENT_CAPACITY);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie que le scan refuse un intervalle vide.
static void test_moon_events_scan_rejects_empty_interval(void) {
  // Given
  const double start_jd = 2461358.5;
  const double end_jd = start_jd;
  MoonEvent events[EVENT_CAPACITY];

  // When
  int status = moon_events_scan(start_jd, end_jd, PARIS_LATITUDE,
                                PARIS_LONGITUDE, events, EVENT_CAPACITY);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie que le scan refuse une capacité nulle.
static void test_moon_events_scan_rejects_zero_capacity(void) {
  // Given
  const double start_jd = 2461358.5;
  const double end_jd = start_jd + 1.0;
  MoonEvent events[EVENT_CAPACITY];

  // When
  int status = moon_events_scan(start_jd, end_jd, PARIS_LATITUDE,
                                PARIS_LONGITUDE, events, 0);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie que le scan refuse une borne non finie.
static void test_moon_events_scan_rejects_non_finite_bound(void) {
  // Given
  const double start_jd = 2461358.5;
  const double end_jd = start_jd + 1.0;
  MoonEvent events[EVENT_CAPACITY];

  // When
  int status = moon_events_scan(NAN, end_jd, PARIS_LATITUDE, PARIS_LONGITUDE,
                                events, EVENT_CAPACITY);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie qu'aucun événement n'est détecté quand la Lune ne se lève pas.
static void test_moon_events_scan_returns_none_for_non_rising_moon(void) {
  // Given
  const double start_jd = 2461358.5 - 1.0 / 24.0;
  const double end_jd = start_jd + 3.0;
  MoonEvent events[EVENT_CAPACITY];

  // When
  int count =
      moon_events_scan(start_jd, end_jd, 89.0, 0.0, events, EVENT_CAPACITY);

  // Then
  TEST_ASSERT_INT_EQ(count, 0);
}

// Vérifie qu'aucun événement n'est détecté quand la Lune est circumpolaire.
static void test_moon_events_scan_returns_none_for_circumpolar_moon(void) {
  // Given
  const double start_jd = 2461368.5 - 1.0 / 24.0;
  const double end_jd = start_jd + 3.0;
  MoonEvent events[EVENT_CAPACITY];

  // When
  int count =
      moon_events_scan(start_jd, end_jd, 89.0, 0.0, events, EVENT_CAPACITY);

  // Then
  TEST_ASSERT_INT_EQ(count, 0);
}

// Vérifie que le type d'événement correspond au sens de la traversée.
static void test_moon_events_scan_labels_rise_and_set_by_altitude_slope(void) {
  // Given
  const double start_jd = 2461358.5 - 1.0 / 24.0;
  const double end_jd = start_jd + 3.0;
  MoonEvent events[EVENT_CAPACITY];
  const double probe_days = 1.0 / 288.0;
  const double tolerance = 1e-9;

  // When
  const int count = moon_events_scan(start_jd, end_jd, PARIS_LATITUDE,
                                     PARIS_LONGITUDE, events, EVENT_CAPACITY);

  // Then
  TEST_ASSERT_TRUE(count >= 4);
  for (int i = 0; i < count; i++) {
    const double before = moon_altitude(events[i].julian_day - probe_days,
                                        PARIS_LATITUDE, PARIS_LONGITUDE) -
                          MOON_HORIZON_ALTITUDE_DEG;
    if (events[i].type == MOON_EVENT_RISE) {
      TEST_ASSERT_TRUE(before < -tolerance);
    } else {
      TEST_ASSERT_TRUE(before > tolerance);
    }
  }
}

// Vérifie qu'un balayage long ne produit ni doublon ni événement hors de
// l'ordre.
static void test_moon_events_scan_long_span_has_no_duplicate_events(void) {
  // Given
  const double start_jd = 2461358.5;
  const double end_jd = start_jd + 40.0;
  MoonEvent events[256];
  const int capacity = (int)(sizeof(events) / sizeof(events[0]));

  // When
  const int count = moon_events_scan(start_jd, end_jd, PARIS_LATITUDE,
                                     PARIS_LONGITUDE, events, capacity);

  // Then
  TEST_ASSERT_TRUE(count >= 40);
  TEST_ASSERT_TRUE(count < capacity);
  for (int i = 0; i < count; i++) {
    const MoonEventType expected =
        (i % 2 == 0) ? MOON_EVENT_RISE : MOON_EVENT_SET;
    TEST_ASSERT_INT_EQ((int)events[i].type, (int)expected);
    if (i > 0) {
      TEST_ASSERT_TRUE(events[i].julian_day > events[i - 1].julian_day);
    }
  }
}

// Vérifie qu'une capacité dépassée est signalée plutôt que tronquée en silence.
static void test_moon_events_scan_reports_truncated_buffer(void) {
  // Given
  const double start_jd = 2461358.5 - 1.0 / 24.0;
  const double end_jd = start_jd + 3.0;
  MoonEvent events[1];

  // When
  const int status = moon_events_scan(start_jd, end_jd, PARIS_LATITUDE,
                                      PARIS_LONGITUDE, events, 1);

  // Then
  TEST_ASSERT_INT_EQ(status, MOON_EVENTS_TRUNCATED);
}

// Vérifie que le scan détecte les événements lorsque la Lune reste
// longuement au voisinage de l'horizon (marge élargie à 1,3°).
static void test_moon_events_scan_detects_events_near_horizon_grazing(void) {
  // Given
  // 2026-11-14, Paris : la Lune se lève et se couche sur la journée.
  const double start_jd = 2461358.5;
  const double end_jd = start_jd + 1.0;
  MoonEvent events[EVENT_CAPACITY];

  // When
  const int count = moon_events_scan(start_jd, end_jd, PARIS_LATITUDE,
                                     PARIS_LONGITUDE, events, EVENT_CAPACITY);

  // Then
  TEST_ASSERT_TRUE(count >= 2);
  TEST_ASSERT_INT_EQ((int)events[0].type, (int)MOON_EVENT_RISE);
  TEST_ASSERT_INT_EQ((int)events[1].type, (int)MOON_EVENT_SET);
}

// Vérifie que la marge élargie ne produit pas de faux événements sur une
// Lune circumpolaire (toujours au-dessus de l'horizon).
static void test_moon_events_scan_wide_margin_no_false_events(void) {
  // Given
  // Date et position circumpolaires : la Lune ne descend jamais sous l'horizon.
  const double start_jd = 2461368.5 - 1.0 / 24.0;
  const double end_jd = start_jd + 3.0;
  MoonEvent events[EVENT_CAPACITY];

  // When
  const int count =
      moon_events_scan(start_jd, end_jd, 89.0, 0.0, events, EVENT_CAPACITY);

  // Then
  TEST_ASSERT_INT_EQ(count, 0);
}

static const TestCase test_cases[] = {
    {"test_moon_events_scan_finds_alternating_events",
     test_moon_events_scan_finds_alternating_events},
    {"test_moon_events_scan_events_are_near_horizon",
     test_moon_events_scan_events_are_near_horizon},
    {"test_moon_events_scan_labels_rise_and_set_by_altitude_slope",
     test_moon_events_scan_labels_rise_and_set_by_altitude_slope},
    {"test_moon_events_scan_long_span_has_no_duplicate_events",
     test_moon_events_scan_long_span_has_no_duplicate_events},
    {"test_moon_events_scan_rejects_null_output",
     test_moon_events_scan_rejects_null_output},
    {"test_moon_events_scan_rejects_empty_interval",
     test_moon_events_scan_rejects_empty_interval},
    {"test_moon_events_scan_rejects_zero_capacity",
     test_moon_events_scan_rejects_zero_capacity},
    {"test_moon_events_scan_rejects_non_finite_bound",
     test_moon_events_scan_rejects_non_finite_bound},
    {"test_moon_events_scan_returns_none_for_non_rising_moon",
     test_moon_events_scan_returns_none_for_non_rising_moon},
    {"test_moon_events_scan_returns_none_for_circumpolar_moon",
     test_moon_events_scan_returns_none_for_circumpolar_moon},
    {"test_moon_events_scan_reports_truncated_buffer",
     test_moon_events_scan_reports_truncated_buffer},
    {"test_moon_events_scan_detects_events_near_horizon_grazing",
     test_moon_events_scan_detects_events_near_horizon_grazing},
    {"test_moon_events_scan_wide_margin_no_false_events",
     test_moon_events_scan_wide_margin_no_false_events},
};

int main(void) {
  return runner_run("moon_events", test_cases, TEST_CASE_COUNT(test_cases));
}