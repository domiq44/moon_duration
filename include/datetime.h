/*
 * datetime.h
 * Représentation date/heure, conversion Julian Day, jour de semaine et
 * validation des composantes de date/heure (calendres julien et grégorien).
 */

#ifndef DATETIME_H
#define DATETIME_H

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} DateTime;

DateTime datetime_make(int year, int month, int day, int hour, int minute,
                       int second);

DateTime datetime_add_days(DateTime datetime, int days);

double datetime_to_julian(DateTime datetime);

// Convertit une DateTime en Julian Day en forçant le calendrier grégorien
// proleptique, y compris avant la réforme de 1582.
double datetime_to_julian_gregorian(DateTime datetime);

// Convertit un Julian Day en date/heure, en choisissant le calendrier julien
// avant la réforme grégorienne et le calendrier grégorien ensuite.
DateTime datetime_from_julian(double julian_day);

int datetime_weekday(DateTime datetime);

int datetime_is_valid(DateTime datetime);

#endif /* DATETIME_H */