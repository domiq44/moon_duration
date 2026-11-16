/*
 * main.c
 * Point d'entrée du programme : orchestration des modules pour afficher le
 * lever, le coucher et la durée de visibilité de la Lune pour une date.
 */

#include <math.h>
#include <stdio.h>

#include "cli.h"
#include "cli_status.h"
#include "clock.h"
#include "datetime.h"
#include "i18n.h"
#include "moon_duration.h"
#include "moon_orientation.h"
#include "moon_phase.h"
#include "report.h"
#include "version.h"

// Plage pour laquelle le modèle lunaire abrégé est garanti (voir
// ALGORITHME.md, §8). Hors de cette plage, un avertissement est émis.
#define MOON_MODEL_MIN_YEAR 1900
#define MOON_MODEL_MAX_YEAR 3000

// Détermine la date du rapport et l'instant de l'aperçu. L'aperçu suit la date
// et l'heure fournies ; sans heure, il utilise midi pour une date explicite et
// l'heure courante sinon.
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static int select_local_date(const CliArgs *args, DateTime *local_date,
                             DateTime *preview_local) {
  if (args->has_date) {
    *local_date = datetime_make(args->date.year, args->date.month,
                                args->date.day, 0, 0, 0);
    *preview_local = datetime_make(
        args->date.year, args->date.month, args->date.day,
        args->has_time ? args->hour : 12, args->has_time ? args->minute : 0, 0);
    return 0;
  }

  ClockTime now;
  if (clock_now(&now) != 0) {
    return -1;
  }

  *local_date =
      datetime_make(now.local.year, now.local.month, now.local.day, 0, 0, 0);
  *preview_local =
      datetime_make(now.local.year, now.local.month, now.local.day,
                    args->has_time ? args->hour : now.local.hour,
                    args->has_time ? args->minute : now.local.minute, 0);
  return 0;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// Convertit une DateTime locale en Julian Day UTC.
static int local_date_to_julian(DateTime local_date, double *julian_day) {
  DateTime utc;

  if (clock_local_to_utc(local_date, &utc) != 0) {
    return -1;
  }

  *julian_day = datetime_to_julian(utc);
  return 0;
}

// Point d'entrée de la commande, exposé pour les tests de bout en bout.
int run_cli(int argc, char *argv[], FILE *out, FILE *err) {
  CliArgs args;
  CliStatus status;
  DateTime local_date;
  DateTime next_local_date;
  DateTime preview_local;
  double day_start_jd;
  double day_end_jd;
  double preview_jd;
  MoonDuration duration;
  MoonDurationReport report;

  status = cli_parse(argc, argv, &args);

  // La langue demandee est appliquee avant tout message, y compris lorsque
  // l'analyse des arguments echoue.
  if (args.lang[0] != '\0') {
    (void)i18n_set_lang(args.lang);
  }

  if (status != CLI_OK) {
    cli_print_error(err, status);
    cli_print_usage(err, argv[0]);
    return 1;
  }

  if (args.show_version) {
    (void)fprintf(out, "moon_duration %s\n", MOON_VERSION);
    return 0;
  }

  if (args.show_help) {
    cli_print_usage(out, argv[0]);
    return 0;
  }

  if (select_local_date(&args, &local_date, &preview_local) != 0) {
    (void)fprintf(err, "%s\n", i18n_error_message(CLI_ERR_CLOCK));
    return 1;
  }

  // La précision du modèle n'est garantie que sur une plage bornée : au-delà,
  // le calcul est conservé mais l'utilisateur est averti.
  if (local_date.year < MOON_MODEL_MIN_YEAR ||
      local_date.year > MOON_MODEL_MAX_YEAR) {
    (void)fprintf(err, "%s\n", i18n_warning_date_out_of_range());
  }

  next_local_date = datetime_add_days(local_date, 1);

  if (local_date_to_julian(local_date, &day_start_jd) != 0 ||
      local_date_to_julian(next_local_date, &day_end_jd) != 0 ||
      local_date_to_julian(preview_local, &preview_jd) != 0) {
    (void)fprintf(err, "%s\n", i18n_error_message(CLI_ERR_CLOCK));
    return 1;
  }

  if (moon_duration_compute(day_start_jd, day_end_jd, args.latitude,
                            args.longitude, &duration) != 0) {
    (void)fprintf(err, "%s\n", i18n_error_message(CLI_ERR_DURATION));
    return 1;
  }

  report.local_date = local_date;
  report.preview_local = preview_local;
  report.latitude = args.latitude;
  report.longitude = args.longitude;
  report.has_rise = duration.has_rise;
  report.has_set = duration.has_set;
  report.all_day = duration.all_day;
  report.none = duration.none;
  report.duration_hours = duration.duration_hours;
  report.phase = moon_phase_from_julian_day(preview_jd);
  report.illumination = moon_illumination(preview_jd);
  report.inclination_deg =
      moon_bright_limb_angle(preview_jd, args.latitude, args.longitude);
  report.has_inclination = isfinite(report.inclination_deg);
  report.rise_local = datetime_make(1970, 1, 1, 0, 0, 0);
  report.set_local = datetime_make(1970, 1, 1, 0, 0, 0);
  report.rise_utc = datetime_make(1970, 1, 1, 0, 0, 0);
  report.set_utc = datetime_make(1970, 1, 1, 0, 0, 0);

  if (duration.has_rise &&
      (clock_julian_to_local(duration.rise_julian_day, &report.rise_local) !=
           0 ||
       clock_julian_to_utc(duration.rise_julian_day, &report.rise_utc) != 0)) {
    (void)fprintf(err, "%s\n", i18n_error_message(CLI_ERR_CLOCK));
    return 1;
  }

  if (duration.has_set &&
      (clock_julian_to_local(duration.set_julian_day, &report.set_local) != 0 ||
       clock_julian_to_utc(duration.set_julian_day, &report.set_utc) != 0)) {
    (void)fprintf(err, "%s\n", i18n_error_message(CLI_ERR_CLOCK));
    return 1;
  }

  report_print(out, &report);
  return 0;
}

int main(int argc, char *argv[]) {
  i18n_init();

  return run_cli(argc, argv, stdout, stderr);
}
