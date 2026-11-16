/*
 * moon_ascii.h
 * Rendu ASCII déterministe de la phase lunaire (disque, zone éclairée, symboles).
 *
 * Le dessin est orienté comme le voit l'observateur : la verticale en haut,
 * l'est à gauche. Le limbe clair pointe dans la direction de l'angle de
 * position fourni (`bright_limb_angle_deg`, en degrés depuis la verticale vers
 * l'est) ; à défaut d'angle fini, il est déduit de la phase (croissante à
 * droite, décroissante à gauche).
 */

#ifndef MOON_ASCII_H
#define MOON_ASCII_H

#include "moon_phase.h"

#define MOON_ASCII_WIDTH 40
#define MOON_ASCII_HEIGHT 9

typedef struct {
    MoonPhase phase;
    double illumination;
    int has_bright_limb_angle;
    double bright_limb_angle_deg;
} MoonAsciiState;

void moon_ascii_draw(char output[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1],
                     MoonAsciiState state);

#endif /* MOON_ASCII_H */