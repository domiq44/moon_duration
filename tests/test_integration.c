/*
 * test_integration.c
 * Tests d'intégration du binaire CLI : aide, version, rapport texte et rendu
 * ASCII.
 */

#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#include "support/test_runner.h"

#ifndef MOON_DURATION_BIN
#define MOON_DURATION_BIN "build/bin/moon_duration"
#endif

// Capture la sortie d'une commande shell.
static int capture_command(const char *command, char *output,
                           size_t output_size) {
  FILE *stream;
  size_t total = 0;
  int status;

  if (output_size == 0) {
    return -1;
  }

  output[0] = '\0';

  // NOLINTNEXTLINE(bugprone-command-processor,cert-env33-c)
  stream = popen(command, "r");
  if (stream == NULL) {
    return -1;
  }

  while (total < output_size - 1 &&
         fgets(output + total, (int)(output_size - total), stream) != NULL) {
    total += strlen(output + total);
  }

  status = pclose(stream);
  if (status == -1 || !WIFEXITED(status)) {
    return -1;
  }

  return WEXITSTATUS(status);
}

static void assert_contains(const char *text, const char *expected) {
  if (strstr(text, expected) == NULL) {
    runner_context_fail(__FILE__, __LINE__, "texte attendu absent : \"%s\"",
                        expected);
  }
}

// Extrait la valeur affichée après un libellé, jusqu'à la fin de ligne.
static void capture_field(const char *output, const char *label, char *buffer,
                          size_t size) {
  const char *start = strstr(output, label);
  const char *end;
  size_t length;

  if (size == 0) {
    return;
  }

  if (start == NULL) {
    buffer[0] = '\0';
    return;
  }

  start += strlen(label);
  while (*start == ' ' || *start == ':') {
    start++;
  }

  end = strchr(start, '\n');
  length = (end == NULL) ? strlen(start) : (size_t)(end - start);
  if (length >= size) {
    length = size - 1;
  }

  memcpy(buffer, start, length);
  buffer[length] = '\0';
}

// Vérifie que le CLI affiche l'aide avec --help.
static void test_cli_help_prints_usage(void) {
  // Given
  const char *command = "MOON_LANG=en " MOON_DURATION_BIN " --help 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_contains(output, "Usage:");
  assert_contains(output, "moon_duration");
  assert_contains(output, "--lang");
  assert_contains(output, "--at");
}

// Vérifie que le CLI affiche les événements pour une date fixée.
static void test_cli_prints_events_for_fixed_date(void) {
  // Given
  const char *command = "TZ=UTC MOON_LANG=en " MOON_DURATION_BIN
                        " --lat 48.8566 --lon 2.3522 2026-11-14 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_contains(output, "2026-11-14");
  assert_contains(output, "Saturday");
  assert_contains(output, "Moonrise");
  assert_contains(output, "Moonset");
  assert_contains(output, "Duration");
  assert_contains(output, "Moon phase");
  assert_contains(output, "Illumination");
  assert_contains(output, "Tilt");
  assert_contains(output, "\xc2\xb0");
}

// Vérifie que le CLI localise la sortie en français.
static void test_cli_prints_french_output(void) {
  // Given
  const char *command = "TZ=UTC MOON_LANG=fr " MOON_DURATION_BIN
                        " --lat 48.8566 --lon 2.3522 2026-11-14 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_contains(output, "Lever de la lune");
  assert_contains(output, "Coucher de la lune");
  assert_contains(output, "Durée");
  assert_contains(output, "Phase lunaire");
  assert_contains(output, "Inclinaison");
}

// Vérifie que le CLI utilise la date courante sans argument.
static void test_cli_uses_current_date_when_no_argument(void) {
  // Given
  const char *command = "TZ=UTC MOON_LANG=en " MOON_DURATION_BIN " 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_contains(output, "Date");
  assert_contains(output, "Moonrise");
  assert_contains(output, "Moonset");
}

// Vérifie que le CLI affiche une durée nulle pour une Lune sous l'horizon.
static void test_cli_prints_zero_duration_for_polar_moon(void) {
  // Given
  const char *command = "TZ=UTC MOON_LANG=en " MOON_DURATION_BIN
                        " --lat 89 --lon 0 2026-11-14 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_contains(output, "Moonrise");
  assert_contains(output, "Moonset");
  assert_contains(output, "None");
  assert_contains(output, "00:00:00 (0.00 h)");
}

// Vérifie que le CLI affiche vingt-quatre heures pour une Lune
// circumpolaire.
static void test_cli_prints_all_day_duration_for_polar_moon(void) {
  // Given
  const char *command = "TZ=UTC MOON_LANG=en " MOON_DURATION_BIN
                        " --lat 89 --lon 0 2026-11-24 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_contains(output, "Moonrise");
  assert_contains(output, "Moonset");
  assert_contains(output, "None");
  assert_contains(output, "24:00:00 (24.00 h)");
}

// Vérifie que l'option --lang est appliquée, et pas seulement analysée :
// MOON_LANG est en français, l'option doit primer.
static void test_cli_applies_lang_option(void) {
  // Given
  const char *command = "TZ=UTC MOON_LANG=fr " MOON_DURATION_BIN
                        " --lang de --lat 48.85 --lon 2.35 2026-11-14 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_contains(output, "Datum");
  assert_contains(output, "Mondaufgang");
  assert_contains(output, "Monduntergang");
}

// Vérifie que le CLI refuse un format de date invalide.
static void test_cli_rejects_invalid_date_format(void) {
  // Given
  const char *command = "MOON_LANG=en " MOON_DURATION_BIN " 2026-11-14abc 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 1);
  assert_contains(output, "Error:");
  assert_contains(output, "Usage:");
}

// Vérifie que le CLI refuse une latitude invalide.
static void test_cli_rejects_invalid_latitude(void) {
  // Given
  const char *command = "MOON_LANG=en " MOON_DURATION_BIN " --lat 91 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 1);
  assert_contains(output, "Error:");
  assert_contains(output, "Usage:");
}

// Vérifie que le CLI affiche la version issue de l'étiquette Git.
static void test_cli_prints_version(void) {
  // Given
  const char *command = MOON_DURATION_BIN " --version 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_contains(output, "moon_duration ");
  TEST_ASSERT_TRUE(strlen(output) > strlen("moon_duration \n"));
}

// Vérifie que l'aperçu suit la date et l'heure fournies.
static void test_cli_prints_preview_for_requested_time(void) {
  // Given
  const char *command =
      "TZ=UTC MOON_LANG=en " MOON_DURATION_BIN " --at 20:30 2026-11-14 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  assert_contains(output, "Preview");
  assert_contains(output, "2026-11-14 20:30");
}

// Vérifie que l'inclinaison de l'aperçu dépend de l'heure fournie.
static void test_cli_preview_changes_inclination_with_time(void) {
  // Given
  const char *command_noon =
      "TZ=UTC MOON_LANG=en " MOON_DURATION_BIN " --at 12:00 2026-11-14 2>&1";
  const char *command_evening =
      "TZ=UTC MOON_LANG=en " MOON_DURATION_BIN " --at 20:30 2026-11-14 2>&1";
  char noon[4096] = {0};
  char evening[4096] = {0};
  char tilt_noon[32] = {0};
  char tilt_evening[32] = {0};

  // When
  int noon_status = capture_command(command_noon, noon, sizeof(noon));
  int evening_status =
      capture_command(command_evening, evening, sizeof(evening));

  // Then
  TEST_ASSERT_INT_EQ(noon_status, 0);
  TEST_ASSERT_INT_EQ(evening_status, 0);
  capture_field(noon, "Tilt", tilt_noon, sizeof(tilt_noon));
  capture_field(evening, "Tilt", tilt_evening, sizeof(tilt_evening));
  TEST_ASSERT_TRUE(tilt_noon[0] != '\0');
  TEST_ASSERT_TRUE(tilt_evening[0] != '\0');
  TEST_ASSERT_TRUE(strcmp(tilt_noon, tilt_evening) != 0);
}

// Vérifie que l'aperçu sans argument porte la date du rapport.
static void test_cli_preview_uses_report_date_without_arguments(void) {
  // Given
  const char *command = "TZ=UTC MOON_LANG=en " MOON_DURATION_BIN " 2>&1";
  char output[4096] = {0};
  char report_date[16] = {0};
  char preview_date[16] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  const char *date_line = strstr(output, "Date");
  const char *preview_line = strstr(output, "Preview");
  TEST_ASSERT_TRUE(date_line != NULL);
  TEST_ASSERT_TRUE(preview_line != NULL);
  TEST_ASSERT_INT_EQ(sscanf(date_line, "Date%*[ :]%15s", report_date), 1);
  TEST_ASSERT_INT_EQ(sscanf(preview_line, "Preview%*[ :]%15s", preview_date),
                     1);
  TEST_ASSERT_STRING_EQ(report_date, preview_date);
}

// Vérifie que le CLI refuse une heure invalide.
static void test_cli_rejects_invalid_time(void) {
  // Given
  const char *command = "MOON_LANG=en " MOON_DURATION_BIN " --at 24:00 2>&1";
  char output[4096] = {0};

  // When
  int status = capture_command(command, output, sizeof(output));

  // Then
  TEST_ASSERT_INT_EQ(status, 1);
  assert_contains(output, "Error:");
  assert_contains(output, "Usage:");
}

static const TestCase test_cases[] = {
    {"test_cli_help_prints_usage", test_cli_help_prints_usage},
    {"test_cli_prints_version", test_cli_prints_version},
    {"test_cli_prints_events_for_fixed_date",
     test_cli_prints_events_for_fixed_date},
    {"test_cli_prints_french_output", test_cli_prints_french_output},
    {"test_cli_prints_zero_duration_for_polar_moon",
     test_cli_prints_zero_duration_for_polar_moon},
    {"test_cli_prints_all_day_duration_for_polar_moon",
     test_cli_prints_all_day_duration_for_polar_moon},
    {"test_cli_uses_current_date_when_no_argument",
     test_cli_uses_current_date_when_no_argument},
    {"test_cli_rejects_invalid_date_format",
     test_cli_rejects_invalid_date_format},
    {"test_cli_rejects_invalid_latitude", test_cli_rejects_invalid_latitude},
    {"test_cli_applies_lang_option", test_cli_applies_lang_option},
    {"test_cli_prints_preview_for_requested_time",
     test_cli_prints_preview_for_requested_time},
    {"test_cli_preview_changes_inclination_with_time",
     test_cli_preview_changes_inclination_with_time},
    {"test_cli_preview_uses_report_date_without_arguments",
     test_cli_preview_uses_report_date_without_arguments},
    {"test_cli_rejects_invalid_time", test_cli_rejects_invalid_time},
};

int main(void) {
  return runner_run("integration", test_cases, TEST_CASE_COUNT(test_cases));
}