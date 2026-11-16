/*
 * test_report.c
 * Tests du rapport texte : événements, durée, jour entier, Lune absente et
 * libellés.
 */

#include "datetime.h"
#include "i18n.h"
#include "moon_phase.h"
#include "report.h"
#include "support/test_runner.h"

#include <stdio.h>
#include <string.h>

// Capture la sortie du rapport dans une chaîne.
static void capture_report(const MoonDurationReport *report, char *buffer,
                           size_t size) {
  FILE *stream = tmpfile();
  size_t length = 0;

  if (stream == NULL) {
    buffer[0] = '\0';
    return;
  }

  report_print(stream, report);
  TEST_ASSERT_INT_EQ(fseek(stream, 0, SEEK_SET), 0);
  length = fread(buffer, 1, size - 1, stream);
  buffer[length] = '\0';
  (void)fclose(stream);
}

// Vérifie que le rapport affiche les événements et la durée pour Paris.
static void test_report_prints_events_and_duration(void) {
  // Given
  MoonDurationReport report = {0};
  char output[4096];

  report.local_date = datetime_make(2026, 11, 14, 0, 0, 0);
  report.latitude = 48.8566;
  report.longitude = 2.3522;
  report.has_rise = 1;
  report.has_set = 1;
  report.rise_local = datetime_make(2026, 11, 14, 12, 51, 46);
  report.rise_utc = datetime_make(2026, 11, 14, 11, 51, 46);
  report.set_local = datetime_make(2026, 11, 14, 20, 53, 51);
  report.set_utc = datetime_make(2026, 11, 14, 19, 53, 51);
  report.duration_hours = 8.0347;
  report.phase = MOON_WAXING_CRESCENT;
  report.illumination = 0.244;

  // When
  i18n_set_lang("en");
  capture_report(&report, output, sizeof(output));

  // Then
  TEST_ASSERT_TRUE(strstr(output, "2026-11-14") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "Saturday") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "2026-11-14 12:51:46") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "2026-11-14 11:51:46") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "2026-11-14 20:53:51") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "2026-11-14 19:53:51") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "08:02:05") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "Waxing Crescent") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "24.4%") != NULL);
}

// Vérifie que le rapport indique l'absence d'événement.
static void test_report_prints_none_when_no_event(void) {
  // Given
  MoonDurationReport report = {0};
  char output[4096];

  report.local_date = datetime_make(2026, 11, 14, 0, 0, 0);
  report.latitude = 89.0;
  report.longitude = 0.0;
  report.has_rise = 0;
  report.has_set = 0;
  report.duration_hours = 0.0;
  report.phase = MOON_NEW;
  report.illumination = 0.0;

  // When
  i18n_set_lang("en");
  capture_report(&report, output, sizeof(output));

  // Then
  TEST_ASSERT_TRUE(strstr(output, "Moonrise") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "Moonset") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "None") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "Unavailable") != NULL);
}

// Vérifie que le rapport affiche vingt-quatre heures pour une Lune
// circumpolaire.
static void test_report_prints_all_day_duration(void) {
  // Given
  MoonDurationReport report = {0};
  char output[4096];

  report.local_date = datetime_make(2026, 11, 25, 0, 0, 0);
  report.latitude = 89.0;
  report.longitude = 0.0;
  report.all_day = 1;
  report.duration_hours = 24.0;
  report.phase = MOON_FULL;
  report.illumination = 1.0;

  // When
  i18n_set_lang("en");
  capture_report(&report, output, sizeof(output));

  // Then
  TEST_ASSERT_TRUE(strstr(output, "24:00:00 (24.00 h)") != NULL);
}

// Vérifie que le rapport affiche zéro heure pour une Lune absente.
static void test_report_prints_zero_duration_when_moon_absent(void) {
  // Given
  MoonDurationReport report = {0};
  char output[4096];

  report.local_date = datetime_make(2026, 11, 14, 0, 0, 0);
  report.latitude = 89.0;
  report.longitude = 0.0;
  report.none = 1;
  report.duration_hours = 0.0;
  report.phase = MOON_NEW;
  report.illumination = 0.0;

  // When
  i18n_set_lang("en");
  capture_report(&report, output, sizeof(output));

  // Then
  TEST_ASSERT_TRUE(strstr(output, "00:00:00 (0.00 h)") != NULL);
}

// Vérifie que le rapport inclut le dessin ASCII.
static void test_report_prints_ascii_drawing(void) {
  // Given
  MoonDurationReport report = {0};
  char output[4096];

  report.local_date = datetime_make(2026, 11, 24, 0, 0, 0);
  report.latitude = 48.8566;
  report.longitude = 2.3522;
  report.has_rise = 1;
  report.has_set = 1;
  report.rise_local = datetime_make(2026, 11, 24, 16, 22, 0);
  report.rise_utc = datetime_make(2026, 11, 24, 15, 22, 0);
  report.set_local = datetime_make(2026, 11, 25, 8, 18, 0);
  report.set_utc = datetime_make(2026, 11, 25, 7, 18, 0);
  report.duration_hours = 15.93;
  report.phase = MOON_FULL;
  report.illumination = 1.0;

  // When
  i18n_set_lang("en");
  capture_report(&report, output, sizeof(output));

  // Then
  TEST_ASSERT_TRUE(strstr(output, "####################") != NULL);
}

// Vérifie que les dates des événements sont affichées.
static void test_report_prints_event_dates(void) {
  // Given
  MoonDurationReport report = {0};
  char output[4096];

  report.local_date = datetime_make(2026, 11, 17, 0, 0, 0);
  report.latitude = 48.8566;
  report.longitude = 2.3522;
  report.has_rise = 1;
  report.has_set = 1;
  report.rise_local = datetime_make(2026, 11, 17, 13, 57, 0);
  report.rise_utc = datetime_make(2026, 11, 17, 12, 57, 0);
  report.set_local = datetime_make(2026, 11, 18, 0, 23, 0);
  report.set_utc = datetime_make(2026, 11, 17, 23, 23, 0);
  report.duration_hours = 10.4333333333;
  report.phase = MOON_FIRST_QUARTER;
  report.illumination = 0.5;

  // When
  i18n_set_lang("en");
  capture_report(&report, output, sizeof(output));

  // Then
  TEST_ASSERT_TRUE(strstr(output, "2026-11-17 13:57:00") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "2026-11-18 00:23:00") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "10:26:00") != NULL);
}

// Vérifie que le rapport affiche l'inclinaison du limbe lorsqu'elle est connue.
static void test_report_prints_inclination_when_available(void) {
  // Given
  MoonDurationReport report = {0};
  char output[4096];

  report.local_date = datetime_make(2026, 11, 14, 0, 0, 0);
  report.latitude = 48.8566;
  report.longitude = 2.3522;
  report.phase = MOON_WAXING_CRESCENT;
  report.illumination = 0.23;
  report.has_inclination = 1;
  report.inclination_deg = 302.4;

  // When
  i18n_set_lang("en");
  capture_report(&report, output, sizeof(output));

  // Then
  TEST_ASSERT_TRUE(strstr(output, "Tilt") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "302.4") != NULL);
}

// Vérifie que le rapport affiche l'instant de l'aperçu.
static void test_report_prints_preview_instant(void) {
  // Given
  MoonDurationReport report = {0};
  char output[4096];

  report.local_date = datetime_make(2026, 11, 14, 0, 0, 0);
  report.preview_local = datetime_make(2026, 11, 14, 20, 30, 0);
  report.latitude = 48.8566;
  report.longitude = 2.3522;
  report.phase = MOON_WAXING_CRESCENT;
  report.illumination = 0.23;

  // When
  i18n_set_lang("en");
  capture_report(&report, output, sizeof(output));

  // Then
  TEST_ASSERT_TRUE(strstr(output, "Preview") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "2026-11-14 20:30") != NULL);
}

// Vérifie que le rapport signale une inclinaison indéterminée.
static void test_report_prints_inclination_unavailable(void) {
  // Given
  MoonDurationReport report = {0};
  char output[4096];

  report.local_date = datetime_make(2026, 11, 14, 0, 0, 0);
  report.latitude = 48.8566;
  report.longitude = 2.3522;
  report.phase = MOON_FULL;
  report.illumination = 1.0;
  report.has_inclination = 0;
  report.inclination_deg = 0.0;

  // When
  i18n_set_lang("en");
  capture_report(&report, output, sizeof(output));

  // Then
  TEST_ASSERT_TRUE(strstr(output, "Tilt") != NULL);
  TEST_ASSERT_TRUE(strstr(output, "Unavailable") != NULL);
}

// Vérifie que le dessin ASCII suit l'inclinaison du limbe.
static void test_report_ascii_follows_inclination(void) {
  // Given
  MoonDurationReport report = {0};
  char output[4096];
  const char *const lit_left_row = "\n                ##::::::";
  const char *const lit_right_row = "\n                ::::::##";

  report.local_date = datetime_make(2026, 11, 14, 0, 0, 0);
  report.latitude = 48.8566;
  report.longitude = 2.3522;
  report.phase = MOON_WAXING_CRESCENT;
  report.illumination = 0.25;
  report.has_inclination = 1;
  report.inclination_deg = 90.0;

  // When
  i18n_set_lang("en");
  capture_report(&report, output, sizeof(output));

  // Then
  TEST_ASSERT_TRUE(strstr(output, lit_left_row) != NULL);
  TEST_ASSERT_TRUE(strstr(output, lit_right_row) == NULL);
}

// Vérifie qu'une sortie nulle est ignorée.
static void test_report_rejects_null_stream(void) {
  // Given
  MoonDurationReport report = {0};

  // When
  report_print(NULL, &report);

  // Then
  TEST_ASSERT_TRUE(1);
}

// Vérifie qu'aucune ligne du rapport ne se termine par une espace.
static void test_report_prints_no_trailing_spaces(void) {
  // Given
  MoonDurationReport report = {0};
  char output[4096];

  report.local_date = datetime_make(2026, 11, 24, 0, 0, 0);
  report.latitude = 48.8566;
  report.longitude = 2.3522;
  report.phase = MOON_FULL;
  report.illumination = 1.0;

  // When
  i18n_set_lang("en");
  capture_report(&report, output, sizeof(output));

  // Then
  const char *line = output;
  while (*line != '\0') {
    const char *end = strchr(line, '\n');
    const size_t length = (end != NULL) ? (size_t)(end - line) : strlen(line);
    TEST_ASSERT_TRUE(length == 0 || line[length - 1] != ' ');
    if (end == NULL) {
      break;
    }
    line = end + 1;
  }
}

static const TestCase test_cases[] = {
    {"test_report_prints_events_and_duration",
     test_report_prints_events_and_duration},
    {"test_report_prints_none_when_no_event",
     test_report_prints_none_when_no_event},
    {"test_report_prints_all_day_duration",
     test_report_prints_all_day_duration},
    {"test_report_prints_zero_duration_when_moon_absent",
     test_report_prints_zero_duration_when_moon_absent},
    {"test_report_prints_ascii_drawing", test_report_prints_ascii_drawing},
    {"test_report_prints_no_trailing_spaces",
     test_report_prints_no_trailing_spaces},
    {"test_report_prints_event_dates", test_report_prints_event_dates},
    {"test_report_prints_inclination_when_available",
     test_report_prints_inclination_when_available},
    {"test_report_prints_inclination_unavailable",
     test_report_prints_inclination_unavailable},
    {"test_report_prints_preview_instant", test_report_prints_preview_instant},
    {"test_report_ascii_follows_inclination",
     test_report_ascii_follows_inclination},
    {"test_report_rejects_null_stream", test_report_rejects_null_stream},
};

int main(void) {
  i18n_init();
  return runner_run("report", test_cases, TEST_CASE_COUNT(test_cases));
}