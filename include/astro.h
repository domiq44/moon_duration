/*
 * astro.h
 * Conventions et conversions astronomiques de base : normalisation des angles,
 * trigonométrie en degrés, obliquité de l'écliptique, passage des coordonnées
 * écliptiques aux coordonnées équatoriales et temps sidéral moyen.
 *
 * Toutes les valeurs d'angle sont exprimées en degrés.
 */

#ifndef ASTRO_H
#define ASTRO_H

/* Ramène un angle dans [0, 360). */
double astro_norm_360(double angle);

/* Ramène un angle dans [-180, 180]. */
double astro_norm_180(double angle);

double astro_sind(double degrees);

double astro_cosd(double degrees);

double astro_atan2d(double y, double x);

/* Arcsinus en degrés, borné dans [-1, 1]. */
double astro_asind(double value);

/* Convertit une longitude et une latitude écliptiques en ascension droite et
 * déclinaison, pour l'équinoxe de la date. Les pointeurs de sortie sont
 * facultatifs : un pointeur NULL n'est pas écrit. */
void astro_ecliptic_to_equatorial(double ecliptic_longitude_deg,
                                  double ecliptic_latitude_deg,
                                  double julian_day, double *right_ascension_deg,
                                  double *declination_deg);

/* Temps sidéral moyen de Greenwich en degrés. */
double astro_sidereal_mean_degrees(double julian_day);

#endif /* ASTRO_H */