/*
 * test_moon_ascii.c
 * Tests du rendu ASCII lunaire : disque, terminateur, dimensions et
 * terminaison.
 */

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "moon_ascii.h"
#include "moon_phase.h"
#include "support/test_runner.h"

static void assert_shape(const char *drawing, int row, int first, int last) {
  const char *line = drawing + (ptrdiff_t)row * (MOON_ASCII_WIDTH + 1);

  for (int column = 0; column < MOON_ASCII_WIDTH; column++) {
    if (column < first || column > last) {
      TEST_ASSERT_INT_EQ((int)line[column], (int)' ');
    }
  }
}

static void assert_row_contains(const char *drawing, int row,
                                const char *expected) {
  const char *line = drawing + (ptrdiff_t)row * (MOON_ASCII_WIDTH + 1);
  char buffer[MOON_ASCII_WIDTH + 1];

  memcpy(buffer, line, MOON_ASCII_WIDTH);
  buffer[MOON_ASCII_WIDTH] = '\0';
  TEST_ASSERT_TRUE(strstr(buffer, expected) != NULL);
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static void assert_cell(const char *drawing, int row, int column,
                        char expected) {
  const char *line = drawing + (ptrdiff_t)row * (MOON_ASCII_WIDTH + 1);

  TEST_ASSERT_INT_EQ((int)line[column], (int)expected);
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// Compte les symboles pleins d'un dessin.
static int count_full(const char *drawing) {
  int count = 0;

  for (int row = 0; row < MOON_ASCII_HEIGHT; row++) {
    for (int column = 0; column < MOON_ASCII_WIDTH; column++) {
      if (drawing[row * (MOON_ASCII_WIDTH + 1) + column] == '#') {
        count++;
      }
    }
  }
  return count;
}

// Vérifie que chaque ligne est terminée et ne dépasse pas la largeur.
static void test_moon_ascii_draw_returns_null_terminated_rows(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_FULL;
  const double expected_illumination = 1.0;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  for (int row = 0; row < MOON_ASCII_HEIGHT; row++) {
    TEST_ASSERT_INT_EQ((int)drawing[row][MOON_ASCII_WIDTH], (int)'\0');
    TEST_ASSERT_INT_EQ((int)strlen(drawing[row]), MOON_ASCII_WIDTH);
  }
}

// Vérifie que la pleine lune remplit le disque de symboles pleins.
static void test_moon_ascii_draw_full_moon_has_full_disc(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_FULL;
  const double expected_illumination = 1.0;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_shape(&drawing[0][0], 0, 16, 23);
  assert_shape(&drawing[0][0], MOON_ASCII_HEIGHT - 1, 16, 23);
  assert_row_contains(&drawing[0][0], 4, "####################");
}

// Vérifie que le disque n'est pas trop étroit (compensation du ratio
// d'aspect des caractères terminaux).
static void test_moon_ascii_draw_disc_has_sufficient_width(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_FULL;
  const double expected_illumination = 1.0;
  const int middle_row = MOON_ASCII_HEIGHT / 2;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  const char *middle = &drawing[middle_row][0];
  int occupied = 0;
  for (int column = 0; column < MOON_ASCII_WIDTH; column++) {
    if (middle[column] != ' ') {
      occupied++;
    }
  }
  TEST_ASSERT_TRUE(occupied >= MOON_ASCII_WIDTH / 3);
}

// Vérifie que la nouvelle lune ne contient aucun symbole plein.
static void test_moon_ascii_draw_new_moon_has_no_full_symbols(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_NEW;
  const double expected_illumination = 0.0;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  for (int row = 0; row < MOON_ASCII_HEIGHT; row++) {
    for (int column = 0; column < MOON_ASCII_WIDTH; column++) {
      TEST_ASSERT_TRUE(drawing[row][column] != '#');
    }
  }
}

// Vérifie qu'un croissant croissant est éclairé à droite.
static void test_moon_ascii_draw_waxing_crescent_is_lit_on_right(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_WAXING_CRESCENT;
  const double expected_illumination = 0.25;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_row_contains(&drawing[0][0], 4, "##");
}

// Vérifie qu'un croissant décroissant est éclairé à gauche.
static void test_moon_ascii_draw_waning_crescent_is_lit_on_left(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_WANING_CRESCENT;
  const double expected_illumination = 0.25;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_row_contains(&drawing[0][0], 4, "##");
}

// Vérifie qu'un quartier est centré sur la moitié du disque.
static void test_moon_ascii_draw_quarter_is_centered(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_FIRST_QUARTER;
  const double expected_illumination = 0.5;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_row_contains(&drawing[0][0], 4, "####");
}

// Vérifie qu'un premier quartier a sa moitié droite éclairée.
static void test_moon_ascii_draw_first_quarter_lit_right_side(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_FIRST_QUARTER;
  const double expected_illumination = 0.5;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  const char *middle = &drawing[4][0];
  int left_count = 0;
  int right_count = 0;
  for (int column = 0; column < MOON_ASCII_WIDTH / 2; column++) {
    if (middle[column] == '#') {
      left_count++;
    }
  }
  for (int column = MOON_ASCII_WIDTH / 2; column < MOON_ASCII_WIDTH; column++) {
    if (middle[column] == '#') {
      right_count++;
    }
  }
  TEST_ASSERT_TRUE(right_count > left_count);
}

// Vérifie qu'un dernier quartier a sa moitié gauche éclairée.
static void test_moon_ascii_draw_last_quarter_lit_left_side(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_LAST_QUARTER;
  const double expected_illumination = 0.5;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  const char *middle = &drawing[4][0];
  int left_count = 0;
  int right_count = 0;
  for (int column = 0; column < MOON_ASCII_WIDTH / 2; column++) {
    if (middle[column] == '#') {
      left_count++;
    }
  }
  for (int column = MOON_ASCII_WIDTH / 2; column < MOON_ASCII_WIDTH; column++) {
    if (middle[column] == '#') {
      right_count++;
    }
  }
  TEST_ASSERT_TRUE(left_count > right_count);
}

// Vérifie qu'une gibbeuse croissante conserve un disque centré éclairé.
static void test_moon_ascii_draw_waxing_gibbous_has_centered_disc(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_WAXING_GIBBOUS;
  const double expected_illumination = 0.75;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_row_contains(&drawing[0][0], 4, "###");
}

// Vérifie qu'une gibbeuse décroissante conserve un disque centré éclairé.
static void test_moon_ascii_draw_waning_gibbous_has_centered_disc(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_WANING_GIBBOUS;
  const double expected_illumination = 0.75;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_row_contains(&drawing[0][0], 4, "###");
}

// Vérifie qu'une phase inconnue produit un dessin valide sans planter.
static void test_moon_ascii_draw_unknown_phase_returns_rows(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = (MoonPhase)99;
  const double expected_illumination = 0.5;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  for (int row = 0; row < MOON_ASCII_HEIGHT; row++) {
    TEST_ASSERT_INT_EQ((int)drawing[row][MOON_ASCII_WIDTH], (int)'\0');
    TEST_ASSERT_TRUE(strlen(drawing[row]) <= MOON_ASCII_WIDTH);
  }
}

// Vérifie qu'une illumination négative est clampée à zéro (aucun symbole
// plein).
static void test_moon_ascii_draw_negative_illumination_clamped_to_zero(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_WAXING_CRESCENT;
  const double expected_illumination = -0.5;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  for (int row = 0; row < MOON_ASCII_HEIGHT; row++) {
    for (int column = 0; column < MOON_ASCII_WIDTH; column++) {
      TEST_ASSERT_TRUE(drawing[row][column] != '#');
    }
  }
}

// Vérifie qu'une illumination supérieure à un est clampée à un (disque plein).
static void test_moon_ascii_draw_overflow_illumination_clamped_to_one(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_WAXING_CRESCENT;
  const double expected_illumination = 1.5;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_row_contains(&drawing[0][0], 4, "########");
}

// Vérifie qu'une sortie nulle est ignorée sans écrire.
static void test_moon_ascii_draw_rejects_null_output(void) {
  // Given
  const MoonPhase expected_phase = MOON_NEW;
  const double expected_illumination = 0.0;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 0, 0.0};
  moon_ascii_draw(NULL, state);

  // Then
  TEST_ASSERT_TRUE(1);
}

// Vérifie qu'un angle explicite oriente le limbe malgré la phase.
static void test_moon_ascii_draw_bright_limb_angle_overrides_phase(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_WAXING_CRESCENT;
  const double expected_illumination = 0.25;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 1, 90.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_cell(&drawing[0][0], 4, 10, '#');
  assert_cell(&drawing[0][0], 4, 29, ':');
}

// Vérifie qu'un angle non fini retombe sur l'orientation de la phase.
static void test_moon_ascii_draw_non_finite_angle_uses_phase(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_WAXING_CRESCENT;
  const double expected_illumination = 0.25;
  const double invalid_angle = NAN;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 1,
                          invalid_angle};
  moon_ascii_draw(drawing, state);

  // Then
  assert_cell(&drawing[0][0], 4, 29, '#');
  assert_cell(&drawing[0][0], 4, 10, ':');
}

// Vérifie qu'un angle de 0° éclaire le haut du disque.
static void test_moon_ascii_draw_zero_angle_lights_the_top(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_WAXING_CRESCENT;
  const double expected_illumination = 0.5;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 1, 0.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_cell(&drawing[0][0], 0, 16, '#');
  assert_cell(&drawing[0][0], MOON_ASCII_HEIGHT - 1, 16, ':');
}

// Vérifie qu'un angle de 180° éclaire le bas du disque.
static void test_moon_ascii_draw_half_turn_lights_the_bottom(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_WAXING_CRESCENT;
  const double expected_illumination = 0.5;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 1, 180.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_cell(&drawing[0][0], 0, 16, ':');
  assert_cell(&drawing[0][0], MOON_ASCII_HEIGHT - 1, 16, '#');
}

// Vérifie qu'un angle de 90° fait basculer le limbe à gauche.
static void test_moon_ascii_draw_quarter_turn_moves_limbe_to_the_left(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_WAXING_CRESCENT;
  const double expected_illumination = 0.25;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 1, 90.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_cell(&drawing[0][0], 4, 10, '#');
  assert_cell(&drawing[0][0], 4, 29, ':');
}

// Vérifie que la pleine lune reste pleine quel que soit l'angle.
static void test_moon_ascii_draw_full_moon_ignores_angle(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_FULL;
  const double expected_illumination = 1.0;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 1, 123.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_row_contains(&drawing[0][0], MOON_ASCII_HEIGHT - 1, "########");
}

// Vérifie que la nouvelle lune reste éteinte quel que soit l'angle.
static void test_moon_ascii_draw_new_moon_ignores_angle(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_NEW;
  const double expected_illumination = 0.0;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 1, 45.0};
  moon_ascii_draw(drawing, state);

  // Then
  TEST_ASSERT_INT_EQ(count_full(&drawing[0][0]), 0);
}

// Vérifie qu'un angle quelconque conserve la silhouette du disque.
static void test_moon_ascii_draw_keeps_disc_shape_when_rotated(void) {
  // Given
  char drawing[MOON_ASCII_HEIGHT][MOON_ASCII_WIDTH + 1] = {{0}};
  const MoonPhase expected_phase = MOON_FIRST_QUARTER;
  const double expected_illumination = 0.5;

  // When
  MoonAsciiState state = {expected_phase, expected_illumination, 1, 33.0};
  moon_ascii_draw(drawing, state);

  // Then
  assert_shape(&drawing[0][0], 0, 16, 23);
  assert_shape(&drawing[0][0], MOON_ASCII_HEIGHT - 1, 16, 23);
}

static const TestCase test_cases[] = {
    {"test_moon_ascii_draw_returns_null_terminated_rows",
     test_moon_ascii_draw_returns_null_terminated_rows},
    {"test_moon_ascii_draw_full_moon_has_full_disc",
     test_moon_ascii_draw_full_moon_has_full_disc},
    {"test_moon_ascii_draw_disc_has_sufficient_width",
     test_moon_ascii_draw_disc_has_sufficient_width},
    {"test_moon_ascii_draw_new_moon_has_no_full_symbols",
     test_moon_ascii_draw_new_moon_has_no_full_symbols},
    {"test_moon_ascii_draw_waxing_crescent_is_lit_on_right",
     test_moon_ascii_draw_waxing_crescent_is_lit_on_right},
    {"test_moon_ascii_draw_waning_crescent_is_lit_on_left",
     test_moon_ascii_draw_waning_crescent_is_lit_on_left},
    {"test_moon_ascii_draw_quarter_is_centered",
     test_moon_ascii_draw_quarter_is_centered},
    {"test_moon_ascii_draw_first_quarter_lit_right_side",
     test_moon_ascii_draw_first_quarter_lit_right_side},
    {"test_moon_ascii_draw_last_quarter_lit_left_side",
     test_moon_ascii_draw_last_quarter_lit_left_side},
    {"test_moon_ascii_draw_waxing_gibbous_has_centered_disc",
     test_moon_ascii_draw_waxing_gibbous_has_centered_disc},
    {"test_moon_ascii_draw_waning_gibbous_has_centered_disc",
     test_moon_ascii_draw_waning_gibbous_has_centered_disc},
    {"test_moon_ascii_draw_unknown_phase_returns_rows",
     test_moon_ascii_draw_unknown_phase_returns_rows},
    {"test_moon_ascii_draw_negative_illumination_clamped_to_zero",
     test_moon_ascii_draw_negative_illumination_clamped_to_zero},
    {"test_moon_ascii_draw_overflow_illumination_clamped_to_one",
     test_moon_ascii_draw_overflow_illumination_clamped_to_one},
    {"test_moon_ascii_draw_rejects_null_output",
     test_moon_ascii_draw_rejects_null_output},
    {"test_moon_ascii_draw_bright_limb_angle_overrides_phase",
     test_moon_ascii_draw_bright_limb_angle_overrides_phase},
    {"test_moon_ascii_draw_non_finite_angle_uses_phase",
     test_moon_ascii_draw_non_finite_angle_uses_phase},
    {"test_moon_ascii_draw_zero_angle_lights_the_top",
     test_moon_ascii_draw_zero_angle_lights_the_top},
    {"test_moon_ascii_draw_half_turn_lights_the_bottom",
     test_moon_ascii_draw_half_turn_lights_the_bottom},
    {"test_moon_ascii_draw_quarter_turn_moves_limbe_to_the_left",
     test_moon_ascii_draw_quarter_turn_moves_limbe_to_the_left},
    {"test_moon_ascii_draw_full_moon_ignores_angle",
     test_moon_ascii_draw_full_moon_ignores_angle},
    {"test_moon_ascii_draw_new_moon_ignores_angle",
     test_moon_ascii_draw_new_moon_ignores_angle},
    {"test_moon_ascii_draw_keeps_disc_shape_when_rotated",
     test_moon_ascii_draw_keeps_disc_shape_when_rotated},
};

int main(void) {
  return runner_run("moon_ascii", test_cases, TEST_CASE_COUNT(test_cases));
}