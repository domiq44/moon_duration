/*
 * moon_ascii.c
 * Rendu ASCII déterministe de la phase lunaire (disque, zone éclairée,
 * symboles). Le disque est une silhouette fixe ; seul le terminateur est
 * orienté, ce qui évite de faire tourner une grille de caractères.
 */

#include "moon_ascii.h"

#include <math.h>
#include <stddef.h>

#include "moon_phase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FULL_CHAR '#'
#define SHADE_CHAR ':'
#define EMPTY_CHAR ' '

/* Géométrie du disque, en coordonnées de la grille. */
#define DISC_CENTER_COLUMN 20.0
#define DISC_HALF_WIDTH 10.0
#define DISC_CENTER_ROW 4.0
#define DISC_HALF_HEIGHT 4.5

/* Orientation par défaut du limbe : croissante à droite, décroissante à
   gauche. */
#define LIMB_WAXING_ANGLE 270.0
#define LIMB_WANING_ANGLE 90.0

static const int disc_extent[MOON_ASCII_HEIGHT] = {8,  12, 16, 18, 20,
                                                   18, 16, 12, 8};

// Indique si la phase est décroissante (dernier quartier, dernier croissant ou
// gibbeuse décroissante).
static int is_waning(MoonPhase phase) {
  return phase == MOON_LAST_QUARTER || phase == MOON_WANING_CRESCENT ||
         phase == MOON_WANING_GIBBOUS;
}

// Clampe l'illumination dans l'intervalle [0.0, 1.0].
static double clamp_illumination(double illumination) {
  if (illumination < 0.0) {
    return 0.0;
  }
  if (illumination > 1.0) {
    return 1.0;
  }
  return illumination;
}

// Direction du limbe clair dans le plan du dessin, pour un angle de position
// compté depuis la verticale vers l'est. L'axe des ordonnées est orienté vers
// le haut : 0° pointe vers le haut, 90° vers la gauche, 270° vers la droite.
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static void limb_direction(double angle_deg, double *x, double *y) {
  const double radians = angle_deg * (M_PI / 180.0);

  *x = -sin(radians);
  *y = cos(radians);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// Angle de position du limbe : l'angle fourni, ou l'orientation déduite de la
// phase lorsque l'orientation n'est pas connue.
static double limb_angle(MoonAsciiState state) {
  if (state.has_bright_limb_angle && isfinite(state.bright_limb_angle_deg)) {
    return state.bright_limb_angle_deg;
  }

  return is_waning(state.phase) ? LIMB_WANING_ANGLE : LIMB_WAXING_ANGLE;
}

// Indique si la cellule appartient au disque.
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static int is_inside_disc(int row, int column) {
  const int extent = disc_extent[row];
  const int left = (MOON_ASCII_WIDTH - extent) / 2;

  return column >= left && column < left + extent;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// Indique si la cellule est éclairée. Le terminateur est une droite dont la
// normale suit le limbe ; la fraction éclairée de la ligne vaut k, comme dans
// le rendu non orienté.
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static int is_lit(int row, int column, MoonPhase phase, double illumination,
                  double limb_x, double limb_y) {
  if (!is_inside_disc(row, column)) {
    return 0;
  }
  if (phase == MOON_NEW) {
    return 0;
  }
  if (phase == MOON_FULL) {
    return 1;
  }

  const double extent = (double)disc_extent[row];
  const double horizontal =
      (column + 0.5 - DISC_CENTER_COLUMN) / (extent / 2.0);
  const double vertical = (DISC_CENTER_ROW - row) / DISC_HALF_HEIGHT;
  const double projection = horizontal * limb_x + vertical * limb_y;

  return projection >= 1.0 - 2.0 * clamp_illumination(illumination);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// Retourne le caractère à afficher pour une cellule (plein, ombré ou vide).
static char cell_at(int row, int column, MoonAsciiState state, double limb_x,
                    double limb_y) {
  if (is_lit(row, column, state.phase, state.illumination, limb_x, limb_y)) {
    return FULL_CHAR;
  }
  if (is_inside_disc(row, column)) {
    return SHADE_CHAR;
  }
  return EMPTY_CHAR;
}

// Dessine la phase lunaire en ASCII (40×9, lignes nul-terminées).
void moon_ascii_draw(char output[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1],
                     MoonAsciiState state) {
  if (output == NULL) {
    return;
  }

  double limb_x;
  double limb_y;
  limb_direction(limb_angle(state), &limb_x, &limb_y);

  for (int row = 0; row < MOON_ASCII_HEIGHT; row++) {
    for (int column = 0; column < MOON_ASCII_WIDTH; column++) {
      output[row][column] = cell_at(row, column, state, limb_x, limb_y);
    }
    output[row][MOON_ASCII_WIDTH] = '\0';
  }
}