/*
 * moon_events.h
 * Détection des levers et couchers de la Lune pour un observateur.
 */

#ifndef MOON_EVENTS_H
#define MOON_EVENTS_H

typedef enum { MOON_EVENT_RISE, MOON_EVENT_SET } MoonEventType;

typedef struct {
    MoonEventType type;
    double julian_day;
} MoonEvent;

#define MOON_HORIZON_ALTITUDE_DEG 0.125

/* Code de retour de moon_events_scan lorsque la capacité du tampon a été
 * atteinte et que des événements ont dû être omis. */
#define MOON_EVENTS_TRUNCATED (-2)

/* Écrit les événements de l'intervalle dans `events`. Retourne le nombre
 * d'événements écrits, -1 si l'entrée est invalide, ou MOON_EVENTS_TRUNCATED
 * si la capacité est dépassée. */
int moon_events_scan(double start_jd, double end_jd, double latitude,
                     double longitude, MoonEvent *events, int capacity);

#endif /* MOON_EVENTS_H */