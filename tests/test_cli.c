/*
 * test_cli.c
 * Tests de l'analyse des arguments CLI : options, validations, précédence de
 * l'aide et de la version.
 */

#include "cli.h"
#include "cli_status.h"
#include "clock.h"
#include "datetime.h"
#include "i18n.h"
#include "support/test_runner.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

// Horloge de test : renvoie l'instant du 14 novembre 2025 à 19:53:20 UTC.
static time_t fixed_clock_source(void) { return (time_t)1763150000; }

static void assert_datetime_eq(DateTime actual, DateTime expected) {
  TEST_ASSERT_INT_EQ(actual.year, expected.year);
  TEST_ASSERT_INT_EQ(actual.month, expected.month);
  TEST_ASSERT_INT_EQ(actual.day, expected.day);
  TEST_ASSERT_INT_EQ(actual.hour, expected.hour);
  TEST_ASSERT_INT_EQ(actual.minute, expected.minute);
  TEST_ASSERT_INT_EQ(actual.second, expected.second);
}

// Vérifie que l'absence d'argument conserve les valeurs par défaut.
static void test_cli_parse_uses_defaults_when_no_argument(void) {
  // Given
  char *argv[] = {"moon_duration"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(1, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_FALSE(args.has_date);
  TEST_ASSERT_FALSE(args.has_time);
  TEST_ASSERT_FALSE(args.show_help);
  TEST_ASSERT_DOUBLE_EQ(args.latitude, 48.8566, 1e-9);
  TEST_ASSERT_DOUBLE_EQ(args.longitude, 2.3522, 1e-9);
}

// Vérifie qu'une date seule est acceptée avec l'heure à minuit.
static void test_cli_parse_accepts_date_only(void) {
  // Given
  char *argv[] = {"moon_duration", "2026-11-14"};
  const DateTime expected = datetime_make(2026, 11, 14, 0, 0, 0);
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_TRUE(args.has_date);
  assert_datetime_eq(args.date, expected);
}

// Vérifie que --help active le mode aide.
static void test_cli_parse_accepts_help(void) {
  // Given
  char *argv[] = {"moon_duration", "--help"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_TRUE(args.show_help);
  TEST_ASSERT_FALSE(args.has_date);
}

// Vérifie que --help l'emporte sur une latitude invalide.
static void test_cli_parse_help_wins_over_invalid_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "999", "--help"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(4, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_TRUE(args.show_help);
}

// Vérifie que --version l'emporte sur une latitude invalide.
static void test_cli_parse_version_wins_over_invalid_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "999", "--version"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(4, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_TRUE(args.show_version);
}

// Vérifie que --help résout tout de même la langue demandée.
static void test_cli_parse_help_uses_requested_language(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang", "fr", "--help"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(4, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_TRUE(args.show_help);
  TEST_ASSERT_STRING_EQ(args.lang, "fr");
}

// Vérifie que --help l'emporte sur une date invalide.
static void test_cli_parse_help_wins_over_invalid_date(void) {
  // Given
  char *argv[] = {"moon_duration", "2026-13-01", "--help"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_TRUE(args.show_help);
}

// Vérifie que --lang sélectionne la langue sans modifier l'état global.
static void test_cli_parse_accepts_language(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang", "fr"};
  CliArgs args;
  CliStatus status;

  // When
  i18n_set_lang("en");
  status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_STRING_EQ(args.lang, "fr");
  TEST_ASSERT_STRING_EQ(i18n_lang_code(), "en");
}

// Vérifie que --lat et --lon acceptent des coordonnées décimales.
static void test_cli_parse_accepts_coordinates(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "40.7128", "--lon", "-74.0060"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(5, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_DOUBLE_EQ(args.latitude, 40.7128, 1e-9);
  TEST_ASSERT_DOUBLE_EQ(args.longitude, -74.0060, 1e-9);
}

// Vérifie que les bornes des coordonnées sont acceptées.
static void test_cli_parse_accepts_coordinate_bounds(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "-90", "--lon", "180"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(5, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_DOUBLE_EQ(args.latitude, -90.0, 1e-9);
  TEST_ASSERT_DOUBLE_EQ(args.longitude, 180.0, 1e-9);
}

// Vérifie qu'une coordonnée en notation exponentielle est acceptée.
static void test_cli_parse_accepts_exponent_coordinates(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "4.88e1", "--lon", "-7.4e0"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(5, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_DOUBLE_EQ(args.latitude, 48.8, 1e-9);
  TEST_ASSERT_DOUBLE_EQ(args.longitude, -7.4, 1e-9);
}

// Vérifie qu'une latitude hexadécimale est refusée.
static void test_cli_parse_rejects_hexadecimal_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "0x1p3"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
}

// Vérifie qu'une latitude infinie est refusée.
static void test_cli_parse_rejects_infinite_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "inf"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
}

// Vérifie qu'une latitude « nan » est refusée.
static void test_cli_parse_rejects_nan_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "nan"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
}

// Vérifie qu'une latitude suivie d'un caractère parasite est refusée.
static void test_cli_parse_rejects_trailing_garbage_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "48.8x"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
}

// Vérifie qu'une latitude réduite à un point est refusée.
static void test_cli_parse_rejects_bare_dot_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "."};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
}

// Vérifie qu'une latitude à exposant incomplet est refusée.
static void test_cli_parse_rejects_dangling_exponent_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "1e"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
}

// Vérifie qu'une latitude hors plage est refusée.
static void test_cli_parse_rejects_invalid_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "90.1"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
}

// Vérifie qu'une longitude hors plage est refusée.
static void test_cli_parse_rejects_invalid_longitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lon", "-180.1"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LONGITUDE);
}

// Vérifie qu'une valeur manquante après --lat est refusée.
static void test_cli_parse_rejects_missing_latitude_value(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
}

// Vérifie qu'une valeur manquante après --lang est refusée.
static void test_cli_parse_rejects_missing_language_value(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LANG);
}

// Vérifie qu'un format de date invalide est refusé.
static void test_cli_parse_rejects_invalid_date_format(void) {
  // Given
  char *argv[] = {"moon_duration", "2026-11-14abc"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_DATE_FORMAT);
}

// Vérifie qu'une date hors plage est refusée.
static void test_cli_parse_rejects_invalid_date_range(void) {
  // Given
  char *argv[] = {"moon_duration", "2026-13-01"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_DATE_RANGE);
}

// Vérifie qu'un 29 février non bissextile est refusé.
static void test_cli_parse_rejects_non_leap_day(void) {
  // Given
  char *argv[] = {"moon_duration", "2026-02-29"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_DATE_RANGE);
}

// Vérifie qu'une option inconnue est refusée.
static void test_cli_parse_rejects_unknown_option(void) {
  // Given
  char *argv[] = {"moon_duration", "--foo"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_UNKNOWN_OPTION);
}

// Vérifie qu'un argument supplémentaire est refusé.
static void test_cli_parse_rejects_extra_arguments(void) {
  // Given
  char *argv[] = {"moon_duration", "2026-11-14", "2026-11-15"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_ARGUMENT_COUNT);
}

// Vérifie qu'un composant de date non numérique est refusé.
static void test_cli_parse_rejects_nonnumeric_date_component(void) {
  // Given
  char *argv[] = {"moon_duration", "2026-xx-14"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_DATE_FORMAT);
}

// Vérifie qu'un séparateur de date manquant est refusé.
static void test_cli_parse_rejects_missing_date_separator(void) {
  // Given
  char *argv[] = {"moon_duration", "2026/11/14"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_DATE_FORMAT);
}

// Vérifie qu'une latitude non numérique est refusée.
static void test_cli_parse_rejects_nonnumeric_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "abc"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
}

// Vérifie qu'une latitude précédée d'un signe est refusée.
static void test_cli_parse_rejects_signed_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", "+48.8"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
}

// Vérifie qu'une latitude précédée d'une espace est refusée.
static void test_cli_parse_rejects_padded_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lat", " 48.8"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
}

// Vérifie qu'une structure d'arguments nulle est refusée.
static void test_cli_parse_rejects_null_args(void) {
  // Given
  char *argv[] = {"moon_duration"};

  // When
  CliStatus status = cli_parse(1, argv, NULL);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_ARGUMENT_COUNT);
}

// Vérifie qu'un code de langue inconnu est refusé.
static void test_cli_parse_rejects_invalid_language(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang", "zh"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LANG);
}

// Vérifie qu'un code de langue trop long est refusé.
static void test_cli_parse_rejects_too_long_language(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang", "english"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LANG);
}

// Vérifie que --version active le mode version.
static void test_cli_parse_accepts_version(void) {
  // Given
  char *argv[] = {"moon_duration", "--version"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_TRUE(args.show_version);
  TEST_ASSERT_FALSE(args.show_help);
}

// Vérifie que --lang est exposé avant la validation des autres options, sans
// modifier l'état global de i18n.
static void test_cli_parse_exposes_language_before_validation(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang", "de", "--lat", "91"};
  CliArgs args;
  CliStatus status;

  // When
  i18n_set_lang("fr");
  status = cli_parse(5, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_LATITUDE);
  TEST_ASSERT_STRING_EQ(args.lang, "de");
  TEST_ASSERT_STRING_EQ(i18n_lang_code(), "fr");
}

// Vérifie que l'usage contient le nom du programme et les options.
static void test_cli_print_usage_contains_program_and_options(void) {
  // Given
  FILE *stream = tmpfile();
  char buffer[1024];
  size_t length;

  if (stream == NULL) {
    TEST_ASSERT_TRUE(0);
    return;
  }

  // When
  i18n_set_lang("en");
  cli_print_usage(stream, "moon_duration");
  TEST_ASSERT_INT_EQ(fseek(stream, 0, SEEK_SET), 0);
  length = fread(buffer, 1, sizeof(buffer) - 1, stream);
  buffer[length] = '\0';
  (void)fclose(stream);

  // Then
  TEST_ASSERT_TRUE(strstr(buffer, "Usage: moon_duration") != NULL);
  TEST_ASSERT_TRUE(strstr(buffer, "--lang") != NULL);
  TEST_ASSERT_TRUE(strstr(buffer, "--lat") != NULL);
  TEST_ASSERT_TRUE(strstr(buffer, "--version") != NULL);
  TEST_ASSERT_TRUE(strstr(buffer, "--lon") != NULL);
  TEST_ASSERT_TRUE(strstr(buffer, "--at") != NULL);
}

// Vérifie qu'un flux d'usage nul est ignoré.
static void test_cli_print_usage_rejects_null_stream(void) {
  // Given
  // When
  cli_print_usage(NULL, "moon_duration");

  // Then
  TEST_ASSERT_TRUE(1);
}

// Vérifie que --at expose l'heure de l'aperçu.
static void test_cli_parse_accepts_time_option(void) {
  // Given
  char *argv[] = {"moon_duration", "--at", "21:30"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_TRUE(args.has_time);
  TEST_ASSERT_INT_EQ(args.hour, 21);
  TEST_ASSERT_INT_EQ(args.minute, 30);
}

// Vérifie que les bornes de l'heure sont acceptées.
static void test_cli_parse_accepts_time_bounds(void) {
  // Given
  char *argv[] = {"moon_duration", "--at", "00:00"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_OK);
  TEST_ASSERT_TRUE(args.has_time);
  TEST_ASSERT_INT_EQ(args.hour, 0);
  TEST_ASSERT_INT_EQ(args.minute, 0);
}

// Vérifie qu'un format d'heure non sur deux chiffres est refusé.
static void test_cli_parse_rejects_invalid_time_format(void) {
  // Given
  char *argv[] = {"moon_duration", "--at", "9:30"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_TIME);
}

// Vérifie qu'une heure hors plage est refusée.
static void test_cli_parse_rejects_time_out_of_range(void) {
  // Given
  char *argv[] = {"moon_duration", "--at", "24:00"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_TIME);
}

// Vérifie qu'une heure précédée d'un signe est refusée.
static void test_cli_parse_rejects_signed_time(void) {
  // Given
  char *argv[] = {"moon_duration", "--at", "+1:30"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_TIME);
}

// Vérifie qu'une heure précédée d'une espace est refusée.
static void test_cli_parse_rejects_padded_time(void) {
  // Given
  char *argv[] = {"moon_duration", "--at", " 1:30"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_TIME);
}

// Vérifie qu'une valeur manquante après --at est refusée.
static void test_cli_parse_rejects_missing_time_value(void) {
  // Given
  char *argv[] = {"moon_duration", "--at"};
  CliArgs args;

  // When
  CliStatus status = cli_parse(2, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, (int)CLI_ERR_TIME);
}

// Vérifie qu'une heure dont les minutes débordent de int est rejetée.
static void test_cli_parse_rejects_minutes_overflowing_int(void) {
  // Given
  char *argv[] = {"moon_duration", "--at", "12:99999999999"};
  CliArgs args;
  const int expected_status = (int)CLI_ERR_TIME;

  // When
  CliStatus status = cli_parse(3, argv, &args);

  // Then
  TEST_ASSERT_INT_EQ((int)status, expected_status);
}

// Exécute la commande complète et capture ses deux flux.
static int capture_run_cli(int argc, char *argv[], char *out, size_t out_size,
                           char *err, size_t err_size) {
  FILE *out_stream;
  FILE *err_stream;
  size_t out_read;
  size_t err_read;

  out[0] = '\0';
  err[0] = '\0';

  out_stream = tmpfile();
  err_stream = tmpfile();
  if (out_stream == NULL || err_stream == NULL) {
    return -1;
  }

  (void)i18n_set_lang("fr");
  clock_set_source(fixed_clock_source);
  int status = run_cli(argc, argv, out_stream, err_stream);
  clock_set_source(NULL);

  (void)fseek(out_stream, 0, SEEK_SET);
  (void)fseek(err_stream, 0, SEEK_SET);
  out_read = fread(out, 1, out_size - 1, out_stream);
  err_read = fread(err, 1, err_size - 1, err_stream);
  out[out_read] = '\0';
  err[err_read] = '\0';

  if (fclose(out_stream) != 0 || fclose(err_stream) != 0) {
    return -1;
  }

  return status;
}

// Vérifie que la date courante vient de l'horloge injectée.
static void test_run_cli_uses_injected_clock_for_current_date(void) {
  // Given
  char *argv[] = {"moon_duration"};
  char out[2048];
  char err[512];
  const char *expected_date = "2025-11-14";
  const int expected_status = 0;

  // When
  int status = capture_run_cli(1, argv, out, sizeof(out), err, sizeof(err));

  // Then
  TEST_ASSERT_INT_EQ(status, expected_status);
  TEST_ASSERT_TRUE(strstr(out, expected_date) != NULL);
  TEST_ASSERT_TRUE(strstr(out, "Aperçu") != NULL);
  TEST_ASSERT_STRING_EQ(err, "");
}

// Vérifie qu'une date explicite fixe l'aperçu à midi sans lire l'horloge.
static void test_run_cli_prints_report_for_given_date(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang", "en", "2026-11-14"};
  char out[2048];
  char err[512];
  const char *expected_preview = "Preview            : 2026-11-14 12:00";
  const int expected_status = 0;

  // When
  int status = capture_run_cli(4, argv, out, sizeof(out), err, sizeof(err));

  // Then
  TEST_ASSERT_INT_EQ(status, expected_status);
  TEST_ASSERT_TRUE(strstr(out, expected_preview) != NULL);
  TEST_ASSERT_STRING_EQ(err, "");
}

// Vérifie qu'une erreur d'analyse écrit l'usage sur le flux d'erreur.
static void test_run_cli_reports_parse_error_on_error_stream(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang", "en", "2026-13-01"};
  char out[2048];
  char err[2048];
  const int expected_status = 1;

  // When
  int status = capture_run_cli(4, argv, out, sizeof(out), err, sizeof(err));

  // Then
  TEST_ASSERT_INT_EQ(status, expected_status);
  TEST_ASSERT_TRUE(strstr(err, "Invalid date") != NULL);
  TEST_ASSERT_STRING_EQ(out, "");
}

// Vérifie que --help est affiché malgré une latitude invalide.
static void test_run_cli_help_wins_over_invalid_latitude(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang", "en", "--lat", "999", "--help"};
  char out[2048];
  char err[512];

  // When
  int status = capture_run_cli(6, argv, out, sizeof(out), err, sizeof(err));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_TRUE(strstr(out, "Usage") != NULL);
  TEST_ASSERT_STRING_EQ(err, "");
}

// Vérifie qu'une date antérieure à 1900 émet un avertissement sur stderr.
static void test_run_cli_warns_for_date_before_1900(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang", "en", "1500-01-01"};
  char out[2048];
  char err[512];

  // When
  int status = capture_run_cli(4, argv, out, sizeof(out), err, sizeof(err));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_TRUE(strstr(err, "Warning") != NULL);
  TEST_ASSERT_TRUE(strstr(out, "1500-01-01") != NULL);
}

// Vérifie qu'une date postérieure à 3000 émet un avertissement sur stderr.
static void test_run_cli_warns_for_date_after_3000(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang", "en", "3500-01-01"};
  char out[2048];
  char err[512];

  // When
  int status = capture_run_cli(4, argv, out, sizeof(out), err, sizeof(err));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_TRUE(strstr(err, "Warning") != NULL);
  TEST_ASSERT_TRUE(strstr(out, "3500-01-01") != NULL);
}

// Vérifie qu'une date dans la plage valide n'émet aucun avertissement.
static void test_run_cli_does_not_warn_for_date_in_range(void) {
  // Given
  char *argv[] = {"moon_duration", "--lang", "en", "2026-11-14"};
  char out[2048];
  char err[512];

  // When
  int status = capture_run_cli(4, argv, out, sizeof(out), err, sizeof(err));

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_STRING_EQ(err, "");
  TEST_ASSERT_TRUE(strstr(out, "2026-11-14") != NULL);
}

static const TestCase test_cases[] = {
    {"test_cli_parse_uses_defaults_when_no_argument",
     test_cli_parse_uses_defaults_when_no_argument},
    {"test_cli_parse_accepts_date_only", test_cli_parse_accepts_date_only},
    {"test_cli_parse_accepts_help", test_cli_parse_accepts_help},
    {"test_cli_parse_help_wins_over_invalid_latitude",
     test_cli_parse_help_wins_over_invalid_latitude},
    {"test_cli_parse_version_wins_over_invalid_latitude",
     test_cli_parse_version_wins_over_invalid_latitude},
    {"test_cli_parse_help_uses_requested_language",
     test_cli_parse_help_uses_requested_language},
    {"test_cli_parse_help_wins_over_invalid_date",
     test_cli_parse_help_wins_over_invalid_date},
    {"test_cli_parse_accepts_language", test_cli_parse_accepts_language},
    {"test_cli_parse_accepts_coordinates", test_cli_parse_accepts_coordinates},
    {"test_cli_parse_accepts_coordinate_bounds",
     test_cli_parse_accepts_coordinate_bounds},
    {"test_cli_parse_accepts_exponent_coordinates",
     test_cli_parse_accepts_exponent_coordinates},
    {"test_cli_parse_rejects_hexadecimal_latitude",
     test_cli_parse_rejects_hexadecimal_latitude},
    {"test_cli_parse_rejects_infinite_latitude",
     test_cli_parse_rejects_infinite_latitude},
    {"test_cli_parse_rejects_nan_latitude",
     test_cli_parse_rejects_nan_latitude},
    {"test_cli_parse_rejects_trailing_garbage_latitude",
     test_cli_parse_rejects_trailing_garbage_latitude},
    {"test_cli_parse_rejects_bare_dot_latitude",
     test_cli_parse_rejects_bare_dot_latitude},
    {"test_cli_parse_rejects_dangling_exponent_latitude",
     test_cli_parse_rejects_dangling_exponent_latitude},
    {"test_cli_parse_accepts_time_option", test_cli_parse_accepts_time_option},
    {"test_cli_parse_accepts_time_bounds", test_cli_parse_accepts_time_bounds},
    {"test_cli_parse_rejects_invalid_time_format",
     test_cli_parse_rejects_invalid_time_format},
    {"test_cli_parse_rejects_time_out_of_range",
     test_cli_parse_rejects_time_out_of_range},
    {"test_cli_parse_rejects_signed_time", test_cli_parse_rejects_signed_time},
    {"test_cli_parse_rejects_padded_time", test_cli_parse_rejects_padded_time},
    {"test_cli_parse_rejects_missing_time_value",
     test_cli_parse_rejects_missing_time_value},
    {"test_cli_parse_rejects_minutes_overflowing_int",
     test_cli_parse_rejects_minutes_overflowing_int},
    {"test_cli_parse_rejects_signed_latitude",
     test_cli_parse_rejects_signed_latitude},
    {"test_cli_parse_rejects_padded_latitude",
     test_cli_parse_rejects_padded_latitude},
    {"test_cli_parse_rejects_invalid_latitude",
     test_cli_parse_rejects_invalid_latitude},
    {"test_cli_parse_rejects_invalid_longitude",
     test_cli_parse_rejects_invalid_longitude},
    {"test_cli_parse_rejects_missing_latitude_value",
     test_cli_parse_rejects_missing_latitude_value},
    {"test_cli_parse_rejects_missing_language_value",
     test_cli_parse_rejects_missing_language_value},
    {"test_cli_parse_rejects_invalid_date_format",
     test_cli_parse_rejects_invalid_date_format},
    {"test_cli_parse_rejects_nonnumeric_date_component",
     test_cli_parse_rejects_nonnumeric_date_component},
    {"test_cli_parse_rejects_missing_date_separator",
     test_cli_parse_rejects_missing_date_separator},
    {"test_cli_parse_rejects_nonnumeric_latitude",
     test_cli_parse_rejects_nonnumeric_latitude},
    {"test_cli_parse_rejects_null_args", test_cli_parse_rejects_null_args},
    {"test_cli_parse_rejects_invalid_date_range",
     test_cli_parse_rejects_invalid_date_range},
    {"test_cli_parse_rejects_non_leap_day",
     test_cli_parse_rejects_non_leap_day},
    {"test_cli_parse_rejects_unknown_option",
     test_cli_parse_rejects_unknown_option},
    {"test_cli_parse_rejects_extra_arguments",
     test_cli_parse_rejects_extra_arguments},
    {"test_cli_parse_rejects_invalid_language",
     test_cli_parse_rejects_invalid_language},
    {"test_cli_parse_exposes_language_before_validation",
     test_cli_parse_exposes_language_before_validation},
    {"test_cli_parse_rejects_too_long_language",
     test_cli_parse_rejects_too_long_language},
    {"test_cli_parse_accepts_version", test_cli_parse_accepts_version},
    {"test_cli_print_usage_contains_program_and_options",
     test_cli_print_usage_contains_program_and_options},
    {"test_cli_print_usage_rejects_null_stream",
     test_cli_print_usage_rejects_null_stream},
    {"test_run_cli_uses_injected_clock_for_current_date",
     test_run_cli_uses_injected_clock_for_current_date},
    {"test_run_cli_prints_report_for_given_date",
     test_run_cli_prints_report_for_given_date},
    {"test_run_cli_help_wins_over_invalid_latitude",
     test_run_cli_help_wins_over_invalid_latitude},
    {"test_run_cli_warns_for_date_before_1900",
     test_run_cli_warns_for_date_before_1900},
    {"test_run_cli_warns_for_date_after_3000",
     test_run_cli_warns_for_date_after_3000},
    {"test_run_cli_does_not_warn_for_date_in_range",
     test_run_cli_does_not_warn_for_date_in_range},
    {"test_run_cli_reports_parse_error_on_error_stream",
     test_run_cli_reports_parse_error_on_error_stream},
};

int main(void) {
  i18n_init();
  return runner_run("cli", test_cases, TEST_CASE_COUNT(test_cases));
}