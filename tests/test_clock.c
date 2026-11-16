/*
 * test_clock.c
 * Tests de l'horloge : heure locale/UTC, source injectée et repli sur
 * l'horloge système.
 */

#include "clock.h"
#include "datetime.h"
#include "support/test_runner.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Sauve le fuseau horaire courant afin de le restaurer après un test.
static void save_tz(char *buffer, size_t size) {
  const char *tz = getenv("TZ");

  if (tz == NULL) {
    buffer[0] = '\0';
    return;
  }

  (void)snprintf(buffer, size, "%s", tz);
}

// Restaure le fuseau horaire sauvegardé.
static void restore_tz(const char *buffer) {
  if (buffer[0] == '\0') {
    (void)unsetenv("TZ");
  } else {
    (void)setenv("TZ", buffer, 1);
  }
  tzset();
}

// Vérifie que deux dates coïncident champ par champ.
static void assert_datetime_equals(DateTime expected, DateTime actual) {
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
  TEST_ASSERT_INT_EQ(actual.hour, expected.hour);
  TEST_ASSERT_INT_EQ(actual.minute, expected.minute);
  TEST_ASSERT_INT_EQ(actual.second, expected.second);
}

// Horloge de test : renvoie l'instant du 14 novembre 2025 à 19:53:20 UTC.
static time_t fixed_clock_source(void) { return (time_t)1763150000; }

// Horloge de test : simule une source indisponible.
static time_t failing_clock_source(void) { return (time_t)-1; }

// Compte les appels à la source d'horloge injectée.
static int clock_source_calls = 0;

static time_t counting_clock_source(void) {
  clock_source_calls++;
  return fixed_clock_source();
}

// Vérifie que l'heure courante fournit des dates locales et UTC valides.
static void test_clock_now_returns_valid_local_and_utc(void) {
  // Given
  ClockTime now;

  // When
  int status = clock_now(&now);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_TRUE(datetime_is_valid(now.local));
  TEST_ASSERT_TRUE(datetime_is_valid(now.utc));
}

// Vérifie qu'une sortie nulle est refusée pour l'heure courante.
static void test_clock_now_rejects_null_output(void) {
  // Given
  // When
  int status = clock_now(NULL);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie que la source d'horloge injectée fixe l'instant observé.
static void test_clock_now_uses_injected_source(void) {
  // Given
  ClockTime now;
  const DateTime expected_utc = datetime_make(2025, 11, 14, 19, 53, 20);
  const int expected_calls = 1;

  // When
  clock_source_calls = 0;
  clock_set_source(counting_clock_source);
  int status = clock_now(&now);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_INT_EQ(clock_source_calls, expected_calls);
  TEST_ASSERT_INT_EQ((int)now.utc.year, expected_utc.year);
  TEST_ASSERT_INT_EQ((int)now.utc.month, expected_utc.month);
  TEST_ASSERT_INT_EQ((int)now.utc.day, expected_utc.day);
  TEST_ASSERT_INT_EQ((int)now.utc.hour, expected_utc.hour);
  TEST_ASSERT_INT_EQ((int)now.utc.minute, expected_utc.minute);
}

// Vérifie qu'une source d'horloge indisponible retombe sur l'horloge système.
static void test_clock_now_falls_back_to_system_clock(void) {
  // Given
  ClockTime now;

  // When
  clock_set_source(failing_clock_source);
  int status = clock_now(&now);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_TRUE(datetime_is_valid(now.local));
  TEST_ASSERT_TRUE(datetime_is_valid(now.utc));

  // When
  clock_set_source(NULL);
  status = clock_now(&now);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
}

// Vérifie qu'une sortie nulle est refusée pour la conversion UTC.
static void test_clock_local_to_utc_rejects_null_output(void) {
  // Given
  const DateTime local = datetime_make(2025, 6, 15, 14, 30, 0);

  // When
  int status = clock_local_to_utc(local, NULL);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie qu'une conversion locale vers UTC réussit et produit une date valide.
static void test_clock_local_to_utc_returns_valid_utc(void) {
  // Given
  const DateTime local = datetime_make(2025, 1, 1, 12, 0, 0);
  DateTime utc;

  // When
  int status = clock_local_to_utc(local, &utc);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_TRUE(datetime_is_valid(utc));
}

// Vérifie qu'une sortie nulle est refusée pour la conversion UTC vers local.
static void test_clock_utc_to_local_rejects_null_output(void) {
  // Given
  const DateTime utc = datetime_make(2025, 6, 15, 12, 0, 0);

  // When
  int status = clock_utc_to_local(utc, NULL);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie qu'une conversion UTC vers locale est inversée par la conversion
// locale vers UTC.
static void test_clock_local_utc_round_trip(void) {
  // Given
  const DateTime utc = datetime_make(2025, 6, 15, 12, 0, 0);
  DateTime local;
  DateTime round_trip;

  // When
  int to_local_status = clock_utc_to_local(utc, &local);
  if (to_local_status != 0) {
    TEST_SKIP("fuseau horaire système indisponible");
  }
  int to_utc_status = clock_local_to_utc(local, &round_trip);

  // Then
  TEST_ASSERT_INT_EQ(to_local_status, 0);
  TEST_ASSERT_INT_EQ(to_utc_status, 0);
  TEST_ASSERT_INT_EQ(round_trip.year, utc.year);
  TEST_ASSERT_INT_EQ(round_trip.month, utc.month);
  TEST_ASSERT_INT_EQ(round_trip.day, utc.day);
  TEST_ASSERT_INT_EQ(round_trip.hour, utc.hour);
  TEST_ASSERT_INT_EQ(round_trip.minute, utc.minute);
  TEST_ASSERT_INT_EQ(round_trip.second, utc.second);
}

// Vérifie que le Julian Day de l'époque Unix est converti en 1970-01-01.
static void test_clock_julian_to_utc_unix_epoch(void) {
  // Given
  const double julian_day = 2440587.5;
  const DateTime expected = datetime_make(1970, 1, 1, 0, 0, 0);

  // When
  DateTime actual = datetime_make(0, 0, 0, 0, 0, 0);
  int status = clock_julian_to_utc(julian_day, &actual);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
  TEST_ASSERT_INT_EQ(actual.hour, expected.hour);
  TEST_ASSERT_INT_EQ(actual.minute, expected.minute);
  TEST_ASSERT_INT_EQ(actual.second, expected.second);
}

// Vérifie que le Julian Day J2000 est converti en 2000-01-01 12:00 UTC.
static void test_clock_julian_to_utc_j2000(void) {
  // Given
  const double julian_day = 2451545.0;
  const DateTime expected = datetime_make(2000, 1, 1, 12, 0, 0);

  // When
  DateTime actual = datetime_make(0, 0, 0, 0, 0, 0);
  int status = clock_julian_to_utc(julian_day, &actual);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
  TEST_ASSERT_INT_EQ(actual.hour, expected.hour);
  TEST_ASSERT_INT_EQ(actual.minute, expected.minute);
  TEST_ASSERT_INT_EQ(actual.second, expected.second);
}

// Vérifie que la conversion locale d'un Julian Day produit une date valide.
static void test_clock_julian_to_local_returns_valid_date(void) {
  // Given
  const double julian_day = 2451545.0;

  // When
  DateTime local = datetime_make(0, 0, 0, 0, 0, 0);
  int status = clock_julian_to_local(julian_day, &local);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_TRUE(datetime_is_valid(local));
}

// Vérifie que la conversion UTC rejette un pointeur de sortie nul.
static void test_clock_julian_to_utc_rejects_null_output(void) {
  // Given
  const double julian_day = 2451545.0;

  // When
  int status = clock_julian_to_utc(julian_day, NULL);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie que la conversion locale rejette un pointeur de sortie nul.
static void test_clock_julian_to_local_rejects_null_output(void) {
  // Given
  const double julian_day = 2451545.0;

  // When
  int status = clock_julian_to_local(julian_day, NULL);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie qu'un décalage horaire positif est retranché du local.
static void test_clock_local_to_utc_applies_positive_offset(void) {
  // Given
  char saved_tz[64];
  const DateTime local = datetime_make(2025, 1, 15, 12, 0, 0);
  const DateTime expected = datetime_make(2025, 1, 15, 11, 0, 0);
  DateTime utc;

  save_tz(saved_tz, sizeof(saved_tz));
  (void)setenv("TZ", "Europe/Paris", 1);
  tzset();

  // When
  int status = clock_local_to_utc(local, &utc);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_datetime_equals(expected, utc);
  restore_tz(saved_tz);
}

// Vérifie qu'un décalage horaire négatif est ajouté au local.
static void test_clock_local_to_utc_applies_negative_offset(void) {
  // Given
  char saved_tz[64];
  const DateTime local = datetime_make(2025, 1, 15, 12, 0, 0);
  const DateTime expected = datetime_make(2025, 1, 15, 17, 0, 0);
  DateTime utc;

  save_tz(saved_tz, sizeof(saved_tz));
  (void)setenv("TZ", "America/New_York", 1);
  tzset();

  // When
  int status = clock_local_to_utc(local, &utc);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_datetime_equals(expected, utc);
  restore_tz(saved_tz);
}

// Vérifie que la conversion UTC vers locale applique le décalage positif.
static void test_clock_utc_to_local_applies_positive_offset(void) {
  // Given
  char saved_tz[64];
  const DateTime utc = datetime_make(2025, 1, 15, 12, 0, 0);
  const DateTime expected = datetime_make(2025, 1, 15, 13, 0, 0);
  DateTime local;

  save_tz(saved_tz, sizeof(saved_tz));
  (void)setenv("TZ", "Europe/Paris", 1);
  tzset();

  // When
  int status = clock_utc_to_local(utc, &local);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_datetime_equals(expected, local);
  restore_tz(saved_tz);
}

// Vérifie que le détour local vers UTC ne décale pas une date pré-grégorienne.
static void test_clock_local_utc_round_trip_pre_gregorian(void) {
  // Given
  char saved_tz[64];
  const DateTime local = datetime_make(1500, 1, 1, 12, 0, 0);
  DateTime utc;
  DateTime round_trip;

  save_tz(saved_tz, sizeof(saved_tz));
  (void)setenv("TZ", "Europe/Paris", 1);
  tzset();

  // When
  const int to_utc_status = clock_local_to_utc(local, &utc);
  const int to_local_status = clock_utc_to_local(utc, &round_trip);

  // Then
  TEST_ASSERT_INT_EQ(to_utc_status, 0);
  TEST_ASSERT_INT_EQ(to_local_status, 0);
  assert_datetime_equals(local, round_trip);
  restore_tz(saved_tz);
}

// Vérifie qu'un Julian Day pré-grégorien revient à sa date julienne.
static void test_clock_julian_to_utc_pre_gregorian(void) {
  // Given
  char saved_tz[64];
  const DateTime expected = datetime_make(1500, 1, 1, 0, 0, 0);
  DateTime actual;

  save_tz(saved_tz, sizeof(saved_tz));
  (void)setenv("TZ", "UTC", 1);
  tzset();

  // When
  const int status = clock_julian_to_utc(2268932.5, &actual);

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_datetime_equals(expected, actual);
  restore_tz(saved_tz);
}

static const TestCase test_cases[] = {
    {"test_clock_now_returns_valid_local_and_utc",
     test_clock_now_returns_valid_local_and_utc},
    {"test_clock_now_rejects_null_output", test_clock_now_rejects_null_output},
    {"test_clock_now_uses_injected_source",
     test_clock_now_uses_injected_source},
    {"test_clock_now_falls_back_to_system_clock",
     test_clock_now_falls_back_to_system_clock},
    {"test_clock_local_to_utc_rejects_null_output",
     test_clock_local_to_utc_rejects_null_output},
    {"test_clock_local_to_utc_returns_valid_utc",
     test_clock_local_to_utc_returns_valid_utc},
    {"test_clock_utc_to_local_rejects_null_output",
     test_clock_utc_to_local_rejects_null_output},
    {"test_clock_local_utc_round_trip", test_clock_local_utc_round_trip},
    {"test_clock_julian_to_utc_unix_epoch",
     test_clock_julian_to_utc_unix_epoch},
    {"test_clock_julian_to_utc_j2000", test_clock_julian_to_utc_j2000},
    {"test_clock_julian_to_local_returns_valid_date",
     test_clock_julian_to_local_returns_valid_date},
    {"test_clock_julian_to_utc_rejects_null_output",
     test_clock_julian_to_utc_rejects_null_output},
    {"test_clock_julian_to_local_rejects_null_output",
     test_clock_julian_to_local_rejects_null_output},
    {"test_clock_local_to_utc_applies_positive_offset",
     test_clock_local_to_utc_applies_positive_offset},
    {"test_clock_local_to_utc_applies_negative_offset",
     test_clock_local_to_utc_applies_negative_offset},
    {"test_clock_utc_to_local_applies_positive_offset",
     test_clock_utc_to_local_applies_positive_offset},
    {"test_clock_local_utc_round_trip_pre_gregorian",
     test_clock_local_utc_round_trip_pre_gregorian},
    {"test_clock_julian_to_utc_pre_gregorian",
     test_clock_julian_to_utc_pre_gregorian},
};

int main(void) {
  return runner_run("clock", test_cases, TEST_CASE_COUNT(test_cases));
}