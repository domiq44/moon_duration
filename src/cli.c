/*
 * cli.c
 * Analyse, validation et messages d'usage des arguments de la ligne de
 * commande. Supporte --lang, --lat, --lon, --at et une date optionnelle.
 */

#include "cli.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli_status.h"
#include "datetime.h"
#include "i18n.h"

#define DEFAULT_LATITUDE 48.8566
#define DEFAULT_LONGITUDE 2.3522

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
// Lit un entier en base dix. Seuls les chiffres sont acceptés : un signe,
// une espace ou un autre caractère provoque une erreur, ce qui rend « 1:30 »
// et « +1:30 » invalides.
static int parse_int(const char **text, int *value, int *digits) {
  const char *cursor = *text;
  int count = 0;
  long parsed = 0;

  while (*cursor >= '0' && *cursor <= '9') {
    parsed = parsed * 10 + (*cursor - '0');
    if (parsed > INT_MAX) {
      return -1;
    }
    cursor++;
    count++;
  }

  if (count == 0) {
    return -1;
  }

  *digits = count;
  *value = (int)parsed;
  *text = cursor;
  return 0;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

static int expect_char(const char **text, char expected) {
  if (**text != expected) {
    return -1;
  }

  (*text)++;
  return 0;
}

static int parse_date(const char *text, DateTime *date) {
  const char *cursor = text;
  int year, month, day;
  int y_digits, m_digits, d_digits;

  if (parse_int(&cursor, &year, &y_digits) != 0 || y_digits != 4 ||
      expect_char(&cursor, '-') != 0 ||
      parse_int(&cursor, &month, &m_digits) != 0 || m_digits != 2 ||
      expect_char(&cursor, '-') != 0 ||
      parse_int(&cursor, &day, &d_digits) != 0 || d_digits != 2 ||
      *cursor != '\0') {
    return -1;
  }

  *date = datetime_make(year, month, day, 0, 0, 0);
  return 0;
}

// Analyse une heure locale au format HH:MM.
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static int parse_time(const char *text, int *hour, int *minute) {
  const char *cursor = text;
  int parsed_hour, parsed_minute;
  int h_digits, m_digits;

  if (parse_int(&cursor, &parsed_hour, &h_digits) != 0 || h_digits != 2 ||
      expect_char(&cursor, ':') != 0 ||
      parse_int(&cursor, &parsed_minute, &m_digits) != 0 || m_digits != 2 ||
      *cursor != '\0') {
    return -1;
  }

  if (parsed_hour < 0 || parsed_hour > 23 || parsed_minute < 0 ||
      parsed_minute > 59) {
    return -1;
  }

  *hour = parsed_hour;
  *minute = parsed_minute;
  return 0;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
// Analyse un nombre décimal selon une grammaire stricte :
// [-]chiffres["."chiffres][("e"|"E")[ "+"|"-"]chiffres].
// Une forme hexadécimale, « inf », « nan », un « + » de tête, une espace ou un
// caractère résiduel sont refusés.
static int parse_double(const char *text, double *value) {
  const char *cursor = text;
  char *end = NULL;
  double parsed;
  int digits = 0;
  int exponent_digits = 0;

  if (*cursor == '-') {
    cursor++;
  }

  while (isdigit((unsigned char)*cursor)) {
    cursor++;
    digits++;
  }

  if (*cursor == '.') {
    cursor++;
    while (isdigit((unsigned char)*cursor)) {
      cursor++;
      digits++;
    }
  }

  if (digits == 0) {
    return -1;
  }

  if (*cursor == 'e' || *cursor == 'E') {
    cursor++;
    if (*cursor == '+' || *cursor == '-') {
      cursor++;
    }
    while (isdigit((unsigned char)*cursor)) {
      cursor++;
      exponent_digits++;
    }
    if (exponent_digits == 0) {
      return -1;
    }
  }

  if (*cursor != '\0') {
    return -1;
  }

  errno = 0;
  parsed = strtod(text, &end);
  if (end == text || errno == ERANGE || !isfinite(parsed)) {
    return -1;
  }

  *value = parsed;
  return 0;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

CliStatus cli_parse(int argc, char **argv, CliArgs *args) {
  int date_index = -1;
  int lang_index = -1;

  if (args == NULL) {
    return CLI_ERR_ARGUMENT_COUNT;
  }

  args->has_date = 0;
  args->has_time = 0;
  args->hour = 0;
  args->minute = 0;
  args->show_help = 0;
  args->show_version = 0;
  args->lang[0] = '\0';
  args->latitude = DEFAULT_LATITUDE;
  args->longitude = DEFAULT_LONGITUDE;
  args->date = datetime_make(1970, 1, 1, 0, 0, 0);

  // Les options d'aide et de version sont prioritaires : elles ne doivent pas
  // échouer à cause d'un autre argument invalide. La langue est résolue de
  // façon tolérante pour que l'aide s'affiche dans la bonne langue.
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0) {
      args->show_help = 1;
    } else if (strcmp(argv[i], "--version") == 0) {
      args->show_version = 1;
    }
  }

  if (args->show_help || args->show_version) {
    for (int i = 1; i + 1 < argc; i++) {
      if (strcmp(argv[i], "--lang") == 0) {
        if (strlen(argv[i + 1]) < sizeof(args->lang) &&
            i18n_is_supported_lang(argv[i + 1])) {
          strncpy(args->lang, argv[i + 1], sizeof(args->lang) - 1);
          args->lang[sizeof(args->lang) - 1] = '\0';
        }
        break;
      }
    }
    return CLI_OK;
  }

  // La langue est resolue avant toute validation afin que l'appelant puisse
  // l'appliquer meme si l'analyse echoue. cli_parse ne modifie aucun etat
  // global : la langue choisie est exposee via args->lang.
  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], "--lang") == 0) {
      lang_index = i;
    }
  }

  if (lang_index > 0) {
    if (strlen(argv[lang_index + 1]) >= sizeof(args->lang) ||
        !i18n_is_supported_lang(argv[lang_index + 1])) {
      return CLI_ERR_LANG;
    }
    strncpy(args->lang, argv[lang_index + 1], sizeof(args->lang) - 1);
    args->lang[sizeof(args->lang) - 1] = '\0';
  }

  if (argc == 1) {
    return CLI_OK;
  }

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--lang") == 0) {
      if (i + 1 >= argc || strlen(argv[i + 1]) >= sizeof(args->lang)) {
        return CLI_ERR_LANG;
      }
      strncpy(args->lang, argv[i + 1], sizeof(args->lang) - 1);
      args->lang[sizeof(args->lang) - 1] = '\0';
      i++;
      continue;
    }

    if (strcmp(argv[i], "--lat") == 0) {
      if (i + 1 >= argc || parse_double(argv[i + 1], &args->latitude) != 0 ||
          args->latitude < -90.0 || args->latitude > 90.0) {
        return CLI_ERR_LATITUDE;
      }
      i++;
      continue;
    }

    if (strcmp(argv[i], "--lon") == 0) {
      if (i + 1 >= argc || parse_double(argv[i + 1], &args->longitude) != 0 ||
          args->longitude < -180.0 || args->longitude > 180.0) {
        return CLI_ERR_LONGITUDE;
      }
      i++;
      continue;
    }

    if (strcmp(argv[i], "--at") == 0) {
      if (i + 1 >= argc ||
          parse_time(argv[i + 1], &args->hour, &args->minute) != 0) {
        return CLI_ERR_TIME;
      }
      args->has_time = 1;
      i++;
      continue;
    }

    if (argv[i][0] == '-' && argv[i][1] != '\0') {
      return CLI_ERR_UNKNOWN_OPTION;
    }

    if (date_index >= 0) {
      return CLI_ERR_ARGUMENT_COUNT;
    }
    date_index = i;
  }

  if (date_index >= 0) {
    args->has_date = 1;
    if (parse_date(argv[date_index], &args->date) != 0) {
      return CLI_ERR_DATE_FORMAT;
    }
    if (!datetime_is_valid(args->date)) {
      return CLI_ERR_DATE_RANGE;
    }
  }

  return CLI_OK;
}

void cli_print_error(FILE *stream, CliStatus status) {
  if (stream == NULL) {
    return;
  }

  const char *msg = i18n_error_message(status);
  if (*msg != '\0') {
    (void)fprintf(stream, "%s\n", msg);
  }
}

void cli_print_usage(FILE *stream, const char *program) {
  if (stream == NULL) {
    return;
  }

  const I18nUsageStrings usage = i18n_usage_strings();

  (void)fprintf(stream, usage.header, program);
  (void)fputc('\n', stream);
  (void)fputs(usage.description, stream);
  (void)fputc('\n', stream);
  (void)fputc('\n', stream);
  (void)fputs(usage.options, stream);
  (void)fputc('\n', stream);
  (void)fputs(usage.lang, stream);
  (void)fputc('\n', stream);
  (void)fputs(usage.lat, stream);
  (void)fputc('\n', stream);
  (void)fputs(usage.lon, stream);
  (void)fputc('\n', stream);
  (void)fputs(usage.at, stream);
  (void)fputc('\n', stream);
  (void)fputs(usage.version, stream);
  (void)fputc('\n', stream);
  (void)fputs(usage.help, stream);
  (void)fputc('\n', stream);
  (void)fputc('\n', stream);
  (void)fprintf(stream, usage.example, program);
  (void)fputc('\n', stream);
}