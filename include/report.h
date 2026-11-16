/*
 * report.h
 * Formatage de la sortie texte et du dessin ASCII du rapport de durée lunaire.
 */

#ifndef REPORT_H
#define REPORT_H

#include <stdio.h>

#include "datetime.h"
#include "moon_phase.h"

typedef struct {
    DateTime local_date;
    DateTime preview_local;
    double latitude;
    double longitude;
    int has_rise;
    int has_set;
    int all_day;
    int none;
    DateTime rise_local;
    DateTime set_local;
    DateTime rise_utc;
    DateTime set_utc;
    double duration_hours;
    MoonPhase phase;
    double illumination;
    int has_inclination;
    double inclination_deg;
} MoonDurationReport;

void report_print(FILE *stream, const MoonDurationReport *report);

#endif /* REPORT_H */