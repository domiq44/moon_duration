/*
 * web_api.h
 * Contrat de l'API web exposée au module WebAssembly (dates, durée,
 * événements, phase, illumination, altitude, orientation et rendu ASCII).
 */

#ifndef WEB_API_H
#define WEB_API_H

/* Renvoie NAN pour une entrée invalide ou une date hors plage. */
double moon_web_julian_day(int year, int month, int day, int hour,
                           double timezone_offset_minutes);

/* Renvoie la durée de visibilité pour une journée à décalage fixe (24 h), ou
 * NAN si la date ou la position est invalide. Pour une journée avec changement
 * d'heure, utiliser moon_web_window. */
double moon_web_duration_hours(int year, int month, int day,
                               double latitude_degrees,
                               double longitude_degrees,
                               double timezone_offset_minutes);

/* Renvoie le mode de durée : 0 = lever et coucher, 1 = Lune visible toute la
 * journée (24 h), 2 = Lune absente toute la journée (0 h), -1 = entrée invalide
 * ou calcul impossible. Les décalages de début et de fin de journée peuvent
 * différer lorsque la journée contient un changement d'heure. */
int moon_web_duration_mode(int year, int month, int day,
                           double latitude_degrees, double longitude_degrees,
                           double start_offset_minutes,
                           double end_offset_minutes);

/* Renvoie NAN si l'événement n'existe pas ou si l'entrée est invalide. */
double moon_web_event_julian_day(int year, int month, int day,
                                 double latitude_degrees,
                                 double longitude_degrees,
                                 double timezone_offset_minutes, int is_rise);

/* Renvoie la fenêtre locale complète (lever et coucher) de la journée,
 * ou NAN si la date, la position ou l'offset est invalide. */
double moon_web_window(int year, int month, int day,
                       double latitude_degrees, double longitude_degrees,
                       double start_offset_minutes, double end_offset_minutes,
                       double *rise_julian_day, double *set_julian_day);

/* Renvoie la phase (0..7) à midi local, ou NAN si l'entrée est invalide. */
double moon_web_phase(int year, int month, int day,
                      double timezone_offset_minutes);

/* Renvoie la phase (0..7) à l'heure locale donnée, ou NAN si l'entrée est
 * invalide. */
double moon_web_phase_at(int year, int month, int day, int hour, int minute,
                         double timezone_offset_minutes);

/* Renvoie l'illumination (0..1) à midi local, ou NAN si l'entrée est invalide. */
double moon_web_illumination(int year, int month, int day,
                             double timezone_offset_minutes);

/* Renvoie l'illumination (0..1) à l'heure locale donnée, ou NAN si l'entrée
 * est invalide. */
double moon_web_illumination_at(int year, int month, int day, int hour,
                                int minute, double timezone_offset_minutes);

/* Renvoie l'altitude lunaire en degrés, ou NAN si l'entrée est invalide. */
double moon_web_altitude(double julian_day, double latitude_degrees,
                         double longitude_degrees);

/* Renvoie l'inclinaison du limbe brillant en degrés à midi local, ou NAN si
 * l'entrée est invalide ou si le limbe n'est pas identifiable. */
double moon_web_bright_limb_angle(int year, int month, int day,
                                  double latitude_degrees,
                                  double longitude_degrees,
                                  double timezone_offset_minutes);

/* Renvoie l'inclinaison du limbe brillant en degrés à l'heure locale donnée,
 * ou NAN si l'entrée est invalide ou si le limbe n'est pas identifiable. */
double moon_web_bright_limb_angle_at(int year, int month, int day, int hour,
                                     int minute, double latitude_degrees,
                                     double longitude_degrees,
                                     double timezone_offset_minutes);

/* Renvoie la taille du tampon requis par moon_web_ascii_into, terminateur
 * compris. */
int moon_web_ascii_size(void);

/* Écrit le rendu ASCII dans `out`, dont la capacité doit valoir au moins
 * moon_web_ascii_size(). Retourne le nombre d'octets écrits hors terminateur,
 * ou -1 si l'entrée est invalide ou la capacité insuffisante. L'inclinaison du
 * limbe (degrés) oriente le terminateur ; NAN laisse la phase décider du sens
 * (croissante à droite, décroissante à gauche). */
int moon_web_ascii_into(int phase, double illumination,
                        double bright_limb_angle_deg, char *out,
                        int capacity);

#endif