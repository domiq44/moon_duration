/*
 * report.c
 * Formatage de la sortie texte et du dessin ASCII du rapport de durée lunaire.
 * Toutes les libellés sont localisées via le module i18n.
 */

#include "report.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "datetime.h"
#include "i18n.h"
#include "moon_ascii.h"
#include "moon_phase_name.h"

#define LABEL_WIDTH 18

// Retourne le nombre de points de code UTF-8 d'une chaîne.
static int utf8_length(const char *text) {
  int length = 0;

  while (*text != '\0') {
    if (((unsigned char)*text & 0xC0U) != 0x80U) {
      length++;
    }
    text++;
  }

  return length;
}

// Affiche un libellé aligné sur LABEL_WIDTH points de code.
static void print_label(FILE *stream, const char *label) {
  int padding = LABEL_WIDTH - utf8_length(label);

  if (padding < 0) {
    padding = 0;
  }

  (void)fputs(label, stream);
  for (int i = 0; i < padding; i++) {
    (void)fputc(' ', stream);
  }
  (void)fputs(" : ", stream);
}

// Retourne l'hémisphère d'une latitude (N/S).
static char latitude_hemisphere(double latitude) {
  return (latitude < 0.0) ? 'S' : 'N';
}

// Retourne l'hémisphère d'une longitude (E/O).
static char longitude_hemisphere(double longitude) {
  return (longitude < 0.0) ? 'W' : 'E';
}

// Affiche une date et une heure locale, avec l'équivalent UTC.
static void print_event(FILE *stream, const char *label, int available,
                        DateTime local, DateTime utc) {
  if (!available) {
    print_label(stream, label);
    (void)fprintf(stream, "%s\n", i18n_label_none());
    return;
  }

  print_label(stream, label);
  (void)fprintf(
      stream,
      "%04d-%02d-%02d %02d:%02d:%02d %s (%04d-%02d-%02d %02d:%02d:%02d UTC)\n",
      local.year, local.month, local.day, local.hour, local.minute,
      local.second, i18n_label_local(), utc.year, utc.month, utc.day, utc.hour,
      utc.minute, utc.second);
}

// Affiche une ligne du dessin ASCII sans ses espaces de fin.
static void print_moon_row(FILE *stream, const char *row) {
  int length = (int)strlen(row);

  while (length > 0 && row[length - 1] == ' ') {
    length--;
  }

  if (length > 0) {
    (void)fwrite(row, 1, (size_t)length, stream);
  }
  (void)fputc('\n', stream);
}

void report_print(FILE *stream, const MoonDurationReport *report) {
  char moon[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1];
  MoonAsciiState ascii_state;

  if (stream == NULL || report == NULL) {
    return;
  }

  ascii_state.phase = report->phase;
  ascii_state.illumination = report->illumination;
  ascii_state.has_bright_limb_angle = report->has_inclination;
  ascii_state.bright_limb_angle_deg = report->inclination_deg;

  print_label(stream, i18n_label_date());
  (void)fprintf(stream, "%04d-%02d-%02d (%s)\n", report->local_date.year,
                report->local_date.month, report->local_date.day,
                i18n_weekday_name(datetime_weekday(report->local_date)));
  print_label(stream, i18n_label_location());
  (void)fprintf(stream, "%.4f %c, %.4f %c\n", fabs(report->latitude),
                latitude_hemisphere(report->latitude), fabs(report->longitude),
                longitude_hemisphere(report->longitude));
  print_label(stream, i18n_label_preview());
  (void)fprintf(stream, "%04d-%02d-%02d %02d:%02d\n",
                report->preview_local.year, report->preview_local.month,
                report->preview_local.day, report->preview_local.hour,
                report->preview_local.minute);
  print_event(stream, i18n_label_moonrise(), report->has_rise,
              report->rise_local, report->rise_utc);
  print_event(stream, i18n_label_moonset(), report->has_set, report->set_local,
              report->set_utc);

  if (report->all_day) {
    print_label(stream, i18n_label_duration());
    (void)fprintf(stream, "%02d:%02d:%02d (%.2f h)\n", 24, 0, 0,
                  report->duration_hours);
  } else if (report->none) {
    print_label(stream, i18n_label_duration());
    (void)fprintf(stream, "%02d:%02d:%02d (%.2f h)\n", 0, 0, 0,
                  report->duration_hours);
  } else if (report->has_rise && report->has_set) {
    const int total_seconds = (int)llround(report->duration_hours * 3600.0);
    const int hours = total_seconds / 3600;
    const int minutes = (total_seconds / 60) % 60;
    const int seconds = total_seconds % 60;

    print_label(stream, i18n_label_duration());
    (void)fprintf(stream, "%02d:%02d:%02d (%.2f h)\n", hours, minutes, seconds,
                  report->duration_hours);
  } else {
    print_label(stream, i18n_label_duration());
    (void)fprintf(stream, "%s\n", i18n_label_unavailable());
  }

  print_label(stream, i18n_label_phase());
  (void)fprintf(stream, "%s\n", moon_phase_name(report->phase));
  print_label(stream, i18n_label_illumination());
  (void)fprintf(stream, "%.1f%%\n", report->illumination * 100.0);
  print_label(stream, i18n_label_inclination());
  if (report->has_inclination) {
    (void)fprintf(stream, "%.1f°\n", report->inclination_deg);
  } else {
    (void)fprintf(stream, "%s\n", i18n_label_unavailable());
  }

  moon_ascii_draw(moon, ascii_state);
  (void)fprintf(stream, "\n");
  for (int row = 0; row < MOON_ASCII_HEIGHT; row++) {
    print_moon_row(stream, moon[row]);
  }
}