/*
 * clock.h
 * Heure courante, conversions local/UTC et conversions Julian Day vers
 * date/heure.
 */

#ifndef CLOCK_H
#define CLOCK_H

#include "datetime.h"

#include <time.h>

typedef struct {
    DateTime local;
    DateTime utc;
} ClockTime;

// Source d'horloge : renvoie l'horodatage Unix courant.
// L'injection permet aux tests de fixer l'instant observé.
typedef time_t (*ClockSource)(void);

void clock_set_source(ClockSource source);

int clock_now(ClockTime *out);

int clock_local_to_utc(DateTime local, DateTime *utc);

int clock_utc_to_local(DateTime utc, DateTime *local);

int clock_julian_to_utc(double julian_day, DateTime *out);

int clock_julian_to_local(double julian_day, DateTime *out);

#endif /* CLOCK_H */