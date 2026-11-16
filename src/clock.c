/*
 * clock.c
 * Heure courante, conversions local/UTC et conversions Julian Day vers
 * date/heure. Les conversions passent par le calendrier grégorien proleptique
 * pour les champs fournis par la bibliothèque C, puis repassent par le
 * calendrier julien avant 1582 lorsque la date le demande.
 *
 * Hypothese : `time_t` est capable de representer les secondes entre 1900 et
 * 3000 (environ 1,3e10 secondes). Toutes les plateformes modernes utilisent un
 * `time_t` signe 64 bits ; sur un `time_t` 32 bits, `julian_to_timestamp`
 * retournerait -1 hors de la plage representable.
 */

#include "clock.h"

#include <math.h>
#include <time.h>

#include "datetime.h"

#define SECONDS_PER_DAY 86400.0
#define UNIX_EPOCH_JD 2440587.5
#define OFFSET_PASSES 3
#define MAX_TIMESTAMP_SECONDS 9.0e18

// Horloge courante : temps système par défaut, remplaçable par les tests.
static ClockSource clock_source = NULL;

void clock_set_source(ClockSource source) { clock_source = source; }

// Renvoie l'horodatage Unix courant. Une source injectée qui échoue laisse
// l'horloge système prendre le relais : `time()` ne renvoie jamais -1.
static time_t clock_timestamp(void) {
  if (clock_source != NULL) {
    time_t timestamp = clock_source();

    if (timestamp != (time_t)-1) {
      return timestamp;
    }
  }

  return time(NULL);
}

// Convertit une structure tm en DateTime.
static DateTime datetime_from_tm(const struct tm *time) {
  DateTime datetime;

  datetime.year = time->tm_year + 1900;
  datetime.month = time->tm_mon + 1;
  datetime.day = time->tm_mday;
  datetime.hour = time->tm_hour;
  datetime.minute = time->tm_min;
  datetime.second = time->tm_sec;
  return datetime;
}

// Convertit un Julian Day en horodatage Unix arrondi à la seconde.
static int julian_to_timestamp(double julian_day, time_t *out) {
  const double seconds = (julian_day - UNIX_EPOCH_JD) * SECONDS_PER_DAY;

  if (!(seconds >= -MAX_TIMESTAMP_SECONDS &&
        seconds <= MAX_TIMESTAMP_SECONDS)) {
    return -1;
  }

  *out = (time_t)llround(seconds);
  return 0;
}

// Décalage local en jours à l'instant donné. Les champs de `struct tm` sont
// grégoriens proleptiques, donc la conversion force ce calendrier pour éviter
// un décalage de dix jours avant la réforme.
static int local_offset_days(double julian_day, double *offset_days) {
  time_t timestamp;
  struct tm utc_time;
  struct tm local_time;

  if (offset_days == NULL || julian_to_timestamp(julian_day, &timestamp) != 0) {
    return -1;
  }

  if (gmtime_r(&timestamp, &utc_time) == NULL ||
      localtime_r(&timestamp, &local_time) == NULL) {
    return -1;
  }

  *offset_days = datetime_to_julian_gregorian(datetime_from_tm(&local_time)) -
                 datetime_to_julian_gregorian(datetime_from_tm(&utc_time));
  return 0;
}

// Récupère l'heure courante locale et UTC.
int clock_now(ClockTime *out) {
  struct tm local_time;
  struct tm utc_time;
  time_t now;

  if (out == NULL) {
    return -1;
  }

  now = clock_timestamp();

  if (localtime_r(&now, &local_time) == NULL) {
    return -1;
  }
  out->local = datetime_from_tm(&local_time);

  if (gmtime_r(&now, &utc_time) == NULL) {
    return -1;
  }
  out->utc = datetime_from_tm(&utc_time);

  return 0;
}

// Convertit une date/heure locale en UTC. Le décalage dépend de l'instant UTC
// (heure d'été), donc la conversion est itérée jusqu'à stabilisation.
int clock_local_to_utc(DateTime local, DateTime *utc) {
  const double local_jd = datetime_to_julian(local);
  double utc_jd = local_jd;

  if (utc == NULL) {
    return -1;
  }

  for (int i = 0; i < OFFSET_PASSES; i++) {
    double offset_days;
    const double previous_jd = utc_jd;

    if (local_offset_days(utc_jd, &offset_days) != 0) {
      return -1;
    }

    utc_jd = local_jd - offset_days;
    if (utc_jd == previous_jd) {
      break;
    }
  }

  {
    const DateTime result = datetime_from_julian(utc_jd);

    if (!datetime_is_valid(result)) {
      return -1;
    }

    *utc = result;
  }

  return 0;
}

// Convertit une date/heure UTC en heure locale.
int clock_utc_to_local(DateTime utc, DateTime *local) {
  if (local == NULL) {
    return -1;
  }

  return clock_julian_to_local(datetime_to_julian(utc), local);
}

// Convertit un Julian Day UTC en date/heure UTC.
int clock_julian_to_utc(double julian_day, DateTime *out) {
  if (out == NULL) {
    return -1;
  }

  {
    const DateTime result = datetime_from_julian(julian_day);

    if (!datetime_is_valid(result)) {
      return -1;
    }

    *out = result;
  }

  return 0;
}

// Convertit un Julian Day UTC en date/heure locale.
int clock_julian_to_local(double julian_day, DateTime *out) {
  double offset_days;

  if (out == NULL || local_offset_days(julian_day, &offset_days) != 0) {
    return -1;
  }

  {
    const DateTime result = datetime_from_julian(julian_day + offset_days);

    if (!datetime_is_valid(result)) {
      return -1;
    }

    *out = result;
  }

  return 0;
}