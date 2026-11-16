/*
 * moon_orientation.h
 * Orientation du limbe brillant de la Lune :
 *   - angle de position du point lumineux dans le repère géocentrique
 *     (depuis le nord céleste, vers l'est) ;
 *   - angle apparent du limbe par rapport à la verticale du lieu, pour un
 *     observateur donné.
 *
 * Les angles sont en degrés. Une valeur NaN signifie que l'orientation n'est
 * pas définie : Julian Day non fini, ou illumination hors de l'intervalle
 * [2 %, 98 %] où le limbe n'est plus identifiable.
 */

#ifndef MOON_ORIENTATION_H
#define MOON_ORIENTATION_H

/* Angle de position du point lumineux, géocentrique, dans [0, 360). */
double moon_bright_limb_angle_geocentric(double julian_day);

/* Angle du limbe brillant par rapport à la verticale du lieu, dans [0, 360),
 * compté depuis la direction du zénith vers l'est (convention des angles de
 * position). Le résultat dépend de l'instant : l'orientation apparente varie
 * d'environ 12° par jour. */
double moon_bright_limb_angle(double julian_day, double latitude_deg,
                              double longitude_deg);

/* Angle parallactique de la Lune (position du zénith par rapport au nord
 * céleste), dans [-180, 180]. */
double moon_parallactic_angle(double julian_day, double latitude_deg,
                              double longitude_deg);

#endif /* MOON_ORIENTATION_H */