/*
 * datetime.c
 * Représentation DateTime, conversion Julian Day, jour de semaine,
 * validation des composantes de date/heure (calendres julien et grégorien).
 */

#include "datetime.h"

#include <limits.h>
#include <math.h>

// Indique si la date relève du calendrier grégorien (après la réforme de 1582).
static int is_gregorian(DateTime datetime) {
  return datetime.year > 1582 ||
         (datetime.year == 1582 &&
          (datetime.month > 10 ||
           (datetime.month == 10 && datetime.day >= 15)));
}

// Indique si l'année est bissextile (règle julienne ou grégorienne selon la
// date).
static int is_leap_year(DateTime datetime) {
  if (!is_gregorian(datetime)) {
    return datetime.year % 4 == 0;
  }

  return (datetime.year % 4 == 0 && datetime.year % 100 != 0) ||
         (datetime.year % 400 == 0);
}

// Retourne le nombre de jours du mois pour la date donnée.
static int days_in_month(DateTime datetime) {
  static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (datetime.month == 2 && is_leap_year(datetime)) {
    return 29;
  }

  return days[datetime.month - 1];
}

// Construit une structure DateTime à partir de ses composantes.
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
DateTime datetime_make(int year, int month, int day, int hour, int minute,
                       int second) {
  DateTime datetime;

  datetime.year = year;
  datetime.month = month;
  datetime.day = day;
  datetime.hour = hour;
  datetime.minute = minute;
  datetime.second = second;
  return datetime;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// Ajoute un nombre de jours à une date en conservant l'heure.
DateTime datetime_add_days(DateTime datetime, int days) {
  // Un mois hors de 1..12 rendrait days_in_month invalide : la date est
  // renvoyee telle quelle plutot que de lire hors des limites du tableau.
  if (datetime.month < 1 || datetime.month > 12) {
    return datetime;
  }

  // Les bornes entières ne peuvent pas être parcourues sans débordement du
  // compteur : la date est renvoyée telle quelle.
  if (days == INT_MIN || days == INT_MAX) {
    return datetime;
  }

  while (days > 0) {
    if (datetime.day < days_in_month(datetime)) {
      datetime.day++;
    } else {
      datetime.day = 1;
      if (datetime.month == 12) {
        datetime.month = 1;
        datetime.year++;
      } else {
        datetime.month++;
      }
    }
    days--;
  }

  while (days < 0) {
    if (datetime.day > 1) {
      datetime.day--;
    } else {
      if (datetime.month == 1) {
        datetime.month = 12;
        datetime.year--;
      } else {
        datetime.month--;
      }
      datetime.day = days_in_month(datetime);
    }
    days++;
  }

  return datetime;
}

// Convertit une DateTime en Julian Day en sélectionnant explicitement le
// calendrier.
// NOLINTBEGIN(bugprone-integer-division)
static double to_julian(DateTime datetime, int gregorian) {
  int a, y, m;
  double day_fraction, jd;

  a = (14 - datetime.month) / 12;
  y = datetime.year + 4800 - a;
  m = datetime.month + 12 * a - 3;

  if (gregorian) {
    jd = (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
  } else {
    jd = (153 * m + 2) / 5 + 365 * y + y / 4 - 32083;
  }

  day_fraction = (datetime.hour - 12) / 24.0 + datetime.minute / 1440.0 +
                 datetime.second / 86400.0;
  return jd + datetime.day + day_fraction;
}
// NOLINTEND(bugprone-integer-division)

// Convertit une DateTime en Julian Day (calendrier julien ou grégorien selon la
// date).
double datetime_to_julian(DateTime datetime) {
  return to_julian(datetime, is_gregorian(datetime));
}

// Convertit une DateTime en Julian Day en forçant le calendrier grégorien.
double datetime_to_julian_gregorian(DateTime datetime) {
  return to_julian(datetime, 1);
}

// Convertit un Julian Day en date/heure. La fraction du jour est convertie en
// heure, minute et seconde.
DateTime datetime_from_julian(double julian_day) {
  long long a, b, c, d, e, month, year;
  double shifted, z_floor, fraction;
  long long z;
  int day;
  int total_seconds;
  DateTime datetime;

  // Un Julian Day hors de la plage couverte (années 1..9999) est renvoyé sous
  // forme d'une date nulle invalide plutôt que de provoquer un débordement.
  if (!isfinite(julian_day) || julian_day < 0.0 || julian_day > 8000000.0) {
    return datetime_make(0, 0, 0, 0, 0, 0);
  }

  shifted = julian_day + 0.5;
  z_floor = floor(shifted);
  fraction = shifted - z_floor;
  z = (long long)z_floor;

  if (z < 2299161LL) {
    a = z;
  } else {
    const long long alpha =
        (long long)floor(((double)z - 1867216.25) / 36524.25);
    a = z + 1 + alpha - alpha / 4;
  }

  b = a + 1524;
  c = (long long)floor(((double)b - 122.1) / 365.25);
  d = (long long)floor(365.25 * (double)c);
  e = (long long)floor(((double)b - (double)d) / 30.6001);

  day = (int)((double)b - (double)d - floor(30.6001 * (double)e));
  month = (e < 14) ? e - 1 : e - 13;
  year = (month > 2) ? c - 4716 : c - 4715;

  total_seconds = (int)llround(fraction * 86400.0);
  if (total_seconds > 86399) {
    total_seconds = 86399;
  }

  datetime.year = (int)year;
  datetime.month = (int)month;
  datetime.day = day;
  datetime.hour = total_seconds / 3600;
  datetime.minute = (total_seconds / 60) % 60;
  datetime.second = total_seconds % 60;
  return datetime;
}

// Retourne le jour de la semaine (0 = dimanche, ..., 6 = samedi) via
// l'algorithme de Zeller.
int datetime_weekday(DateTime datetime) {
  int gregorian, q, m, k, j, h;
  int year = datetime.year;
  int month = datetime.month;

  gregorian = is_gregorian(datetime);

  if (month < 3) {
    m = month + 12;
    year -= 1;
  } else {
    m = month;
  }

  q = datetime.day;
  k = year % 100;
  j = year / 100;

  if (gregorian) {
    h = (q + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
  } else {
    // Congruence de Zeller pour le calendrier julien : le terme de siecle est
    // 5 - j, et non 5 * j.
    h = (q + (13 * (m + 1)) / 5 + k + k / 4 + 5 - j) % 7;
  }

  h = ((h % 7) + 7) % 7;

  return (h + 6) % 7;
}

// Vérifie la cohérence d'une DateTime (plages, bissextile, réforme
// grégorienne).
int datetime_is_valid(DateTime datetime) {
  if (datetime.year < 1 || datetime.year > 9999) {
    return 0;
  }
  if (datetime.month < 1 || datetime.month > 12) {
    return 0;
  }
  if (datetime.day < 1 || datetime.day > days_in_month(datetime)) {
    return 0;
  }
  if (datetime.hour < 0 || datetime.hour > 23) {
    return 0;
  }
  if (datetime.minute < 0 || datetime.minute > 59) {
    return 0;
  }
  if (datetime.second < 0 || datetime.second > 59) {
    return 0;
  }

  return 1;
}