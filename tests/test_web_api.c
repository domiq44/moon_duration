/*
 * test_web_api.c
 * Tests de l'API web : jour julien, durée, événements, phase, illumination,
 * orientation et ASCII.
 */

#include <math.h>
#include <string.h>

#include "moon_ascii.h"
#include "moon_phase.h"
#include "support/test_runner.h"
#include "web_api.h"

// Capacité de tampon utilisée par les tests du rendu ASCII.
#define ASCII_BUFFER_CAPACITY 512

#define PARIS_LATITUDE 48.8566
#define PARIS_LONGITUDE 2.3522
#define PARIS_OFFSET_MINUTES 60.0
#define JD_2026_11_14 2461358.5

// Vérifie la conversion d'une date locale en Julian Day UTC.
static void test_web_julian_day_utc_midnight(void) {
  // Given
  const double expected_jd = JD_2026_11_14;

  // When
  const double actual_jd = moon_web_julian_day(2026, 11, 14, 0, 0.0);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual_jd, expected_jd, 1e-9);
}

// Vérifie la conversion d'une date locale avec fuseau horaire.
static void test_web_julian_day_timezone_offset(void) {
  // Given
  const double expected_positive_offset_jd = JD_2026_11_14 - 60.0 / 1440.0;
  const double expected_negative_offset_jd = JD_2026_11_14 + 300.0 / 1440.0;

  // When
  const double actual_positive_offset_jd =
      moon_web_julian_day(2026, 11, 14, 0, 60.0);
  const double actual_negative_offset_jd =
      moon_web_julian_day(2026, 11, 14, 0, -300.0);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual_positive_offset_jd, expected_positive_offset_jd,
                        1e-9);
  TEST_ASSERT_DOUBLE_EQ(actual_negative_offset_jd, expected_negative_offset_jd,
                        1e-9);
}

// Vérifie la durée lunaire calculée pour Paris le 14 novembre 2026.
static void test_web_duration_paris_november_14(void) {
  // Given
  const double expected_hours = 8.0364;

  // When
  const double actual_hours = moon_web_duration_hours(
      2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual_hours, expected_hours, 0.01);
}

// Vérifie les instants de lever et de coucher pour Paris le 14 novembre 2026.
static void test_web_event_julian_day_paris_november_14(void) {
  // Given
  const double expected_rise_jd = JD_2026_11_14 + 11.862222 / 24.0;
  const double expected_set_jd = JD_2026_11_14 + 19.898333 / 24.0;

  // When
  const double actual_rise_jd = moon_web_event_julian_day(
      2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES, 1);
  const double actual_set_jd = moon_web_event_julian_day(
      2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES, 0);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual_rise_jd, expected_rise_jd, 1e-4);
  TEST_ASSERT_DOUBLE_EQ(actual_set_jd, expected_set_jd, 1e-4);
}

// Vérifie la phase et l'illumination à midi local pour Paris le 14 novembre
// 2026. L'illumination (0,2273) suit l'élongation Lune-Soleil, avec l'équation
// du centre du Soleil, conforme aux coordonnées apparentes JPL.
static void test_web_phase_and_illumination_paris_november_14(void) {
  // Given
  const int expected_phase = MOON_WAXING_CRESCENT;
  const double expected_illumination = 0.2273;
  const double tolerance = 0.005;

  // When
  const double actual_phase =
      moon_web_phase(2026, 11, 14, PARIS_OFFSET_MINUTES);
  const double actual_illumination =
      moon_web_illumination(2026, 11, 14, PARIS_OFFSET_MINUTES);

  // Then
  TEST_ASSERT_INT_EQ((int)actual_phase, expected_phase);
  TEST_ASSERT_DOUBLE_EQ(actual_illumination, expected_illumination, tolerance);
}

// Vérifie que l'altitude lunaire au lever est proche de l'horizon de référence.
static void test_web_altitude_at_rise(void) {
  // Given
  const double rise_jd = moon_web_event_julian_day(
      2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES, 1);

  // When
  const double actual_altitude =
      moon_web_altitude(rise_jd, PARIS_LATITUDE, PARIS_LONGITUDE);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual_altitude, 0.125, 0.01);
}

// Vérifie que l'inclinaison du limbe est exposée à midi local
// (2026-11-14 11:00 UTC pour Paris, 2461358.9583).
static void test_web_bright_limb_angle_at_noon(void) {
  // Given
  const double expected_angle = 302.25;
  const double tolerance = 0.05;

  // When
  const double actual = moon_web_bright_limb_angle(
      2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual, expected_angle, tolerance);
}

// Vérifie que l'inclinaison du limbe est indéterminée à la pleine lune.
static void test_web_bright_limb_angle_is_nan_at_full_moon(void) {
  // Given
  const double full_moon_day = 25.0;

  // When
  const double actual = moon_web_bright_limb_angle(
      2026, 11, 14 + (int)full_moon_day - 14, PARIS_LATITUDE, PARIS_LONGITUDE,
      PARIS_OFFSET_MINUTES);

  // Then
  TEST_ASSERT_TRUE(isnan(actual));
}

// Vérifie le rejet des entrées invalides pour l'inclinaison du limbe.
static void test_web_bright_limb_angle_rejects_invalid_inputs(void) {
  // Given
  const int out_of_range_year = 1800;
  const int invalid_month = 2;
  const int invalid_day = 30;
  const double invalid_latitude = 91.0;
  const double invalid_offset = 2000.0;

  // When
  const double invalid_date =
      moon_web_bright_limb_angle(out_of_range_year, 11, 14, PARIS_LATITUDE,
                                 PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);
  const double invalid_calendar = moon_web_bright_limb_angle(
      2026, invalid_month, invalid_day, PARIS_LATITUDE, PARIS_LONGITUDE,
      PARIS_OFFSET_MINUTES);
  const double invalid_latitude_angle = moon_web_bright_limb_angle(
      2026, 11, 14, invalid_latitude, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);
  const double invalid_offset_angle = moon_web_bright_limb_angle(
      2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, invalid_offset);

  // Then
  TEST_ASSERT_TRUE(isnan(invalid_date));
  TEST_ASSERT_TRUE(isnan(invalid_calendar));
  TEST_ASSERT_TRUE(isnan(invalid_latitude_angle));
  TEST_ASSERT_TRUE(isnan(invalid_offset_angle));
}

// Vérifie le rejet des entrées invalides.
static void test_web_invalid_inputs(void) {
  // Given
  const int invalid_month = 2;
  const int invalid_day = 30;
  const int invalid_hour = 25;
  const double invalid_latitude = 91.0;
  const double invalid_offset = 2000.0;
  const int invalid_event = 2;

  // When
  const double invalid_calendar =
      moon_web_julian_day(2026, invalid_month, invalid_day, 0, 0.0);
  const double invalid_hour_jd =
      moon_web_julian_day(2026, 11, 14, invalid_hour, 0.0);
  const double invalid_latitude_duration = moon_web_duration_hours(
      2026, 11, 14, invalid_latitude, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);
  const double invalid_offset_duration = moon_web_duration_hours(
      2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, invalid_offset);
  const double invalid_event_jd =
      moon_web_event_julian_day(2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE,
                                PARIS_OFFSET_MINUTES, invalid_event);
  const double invalid_altitude = moon_web_altitude(0.0, invalid_latitude, 0.0);

  // Then
  TEST_ASSERT_TRUE(isnan(invalid_calendar));
  TEST_ASSERT_TRUE(isnan(invalid_hour_jd));
  TEST_ASSERT_TRUE(isnan(invalid_latitude_duration));
  TEST_ASSERT_TRUE(isnan(invalid_offset_duration));
  TEST_ASSERT_TRUE(isnan(invalid_event_jd));
  TEST_ASSERT_TRUE(isnan(invalid_altitude));
}

// Vérifie qu'une Lune sans lever ni coucher retourne NAN.
static void test_web_polar_moon_returns_nan(void) {
  // Given
  const double polar_latitude = 89.0;
  const double polar_longitude = 0.0;
  const double polar_offset = 0.0;

  // When
  const double duration = moon_web_duration_hours(
      2026, 11, 14, polar_latitude, polar_longitude, polar_offset);
  const double rise = moon_web_event_julian_day(
      2026, 11, 14, polar_latitude, polar_longitude, polar_offset, 1);
  const double set = moon_web_event_julian_day(
      2026, 11, 14, polar_latitude, polar_longitude, polar_offset, 0);

  // Then
  TEST_ASSERT_TRUE(isnan(duration));
  TEST_ASSERT_TRUE(isnan(rise));
  TEST_ASSERT_TRUE(isnan(set));
}

// Vérifie que la phase et l'illumination refusent une date invalide.
static void test_web_phase_and_illumination_reject_invalid_date(void) {
  // Given
  const int invalid_month = 2;
  const int invalid_day = 30;
  const double invalid_offset = 0.0;

  // When
  const double phase =
      moon_web_phase(2026, invalid_month, invalid_day, invalid_offset);
  const double illumination =
      moon_web_illumination(2026, invalid_month, invalid_day, invalid_offset);

  // Then
  TEST_ASSERT_TRUE(isnan(phase));
  TEST_ASSERT_TRUE(isnan(illumination));
}

// Vérifie que le rendu ASCII clamp l'illumination hors bornes.
static void test_web_ascii_clamps_illumination(void) {
  // Given
  const double negative_illumination = -0.5;
  const double overflow_illumination = 1.5;
  char negative[ASCII_BUFFER_CAPACITY];
  char overflow[ASCII_BUFFER_CAPACITY];

  // When
  const int negative_length = moon_web_ascii_into(
      MOON_FULL, negative_illumination, NAN, negative, (int)sizeof(negative));
  const int overflow_length = moon_web_ascii_into(
      MOON_FULL, overflow_illumination, NAN, overflow, (int)sizeof(overflow));

  // Then
  TEST_ASSERT_TRUE(negative_length > 0);
  TEST_ASSERT_TRUE(overflow_length > 0);
  TEST_ASSERT_TRUE(strchr(negative, '#') != NULL);
  TEST_ASSERT_TRUE(strchr(overflow, '#') != NULL);
}

// Vérifie le rendu ASCII et le rejet d'une phase invalide.
static void test_web_ascii(void) {
  // Given
  const int full_phase = MOON_FULL;
  const int invalid_phase = -1;
  const double full_illumination = 1.0;
  char full_moon_ascii[ASCII_BUFFER_CAPACITY];
  char invalid_phase_ascii[ASCII_BUFFER_CAPACITY];

  // When
  const int full_length =
      moon_web_ascii_into(full_phase, full_illumination, NAN, full_moon_ascii,
                          (int)sizeof(full_moon_ascii));
  const int invalid_length = moon_web_ascii_into(
      invalid_phase, full_illumination, NAN, invalid_phase_ascii,
      (int)sizeof(invalid_phase_ascii));

  // Then
  TEST_ASSERT_TRUE(full_length > 0);
  TEST_ASSERT_INT_EQ(invalid_length, -1);
  TEST_ASSERT_STRING_EQ(invalid_phase_ascii, "");
  TEST_ASSERT_TRUE(strchr(full_moon_ascii, '#') != NULL);
  TEST_ASSERT_TRUE(strchr(full_moon_ascii, '\n') != NULL);
}

// Vérifie que moon_web_ascii_size annonce la taille du tampon et qu'une
// capacité insuffisante ou un tampon nul sont refusés.
static void test_web_ascii_size_and_capacity(void) {
  // Given
  char buffer[ASCII_BUFFER_CAPACITY];
  const int size = moon_web_ascii_size();

  // When
  const int written = moon_web_ascii_into(MOON_FULL, 1.0, NAN, buffer, size);
  const int refused =
      moon_web_ascii_into(MOON_FULL, 1.0, NAN, buffer, size - 1);
  const int null_buffer = moon_web_ascii_into(MOON_FULL, 1.0, NAN, NULL, size);

  // Then
  TEST_ASSERT_INT_EQ(written, size - 1);
  TEST_ASSERT_INT_EQ(refused, -1);
  TEST_ASSERT_INT_EQ(null_buffer, -1);
}

// Vérifie que l'angle fourni oriente le limbe du rendu ASCII.
static void test_web_ascii_uses_bright_limb_angle(void) {
  // Given
  const double lit_right_angle = 270.0;
  const double lit_left_angle = 90.0;
  const size_t row_length = (size_t)MOON_ASCII_WIDTH + 1;
  const size_t middle_row = MOON_ASCII_HEIGHT / 2;
  char lit_right[ASCII_BUFFER_CAPACITY];
  char lit_left[ASCII_BUFFER_CAPACITY];
  const int disc_left = 10;
  const int disc_right = 29;

  // When
  const int right_length =
      moon_web_ascii_into(MOON_WAXING_CRESCENT, 0.25, lit_right_angle,
                          lit_right, (int)sizeof(lit_right));
  const int left_length =
      moon_web_ascii_into(MOON_WAXING_CRESCENT, 0.25, lit_left_angle, lit_left,
                          (int)sizeof(lit_left));

  // Then
  TEST_ASSERT_TRUE(right_length > 0);
  TEST_ASSERT_TRUE(left_length > 0);
  const char *const right_line = lit_right + middle_row * row_length;
  const char *const left_line = lit_left + middle_row * row_length;
  TEST_ASSERT_INT_EQ((int)right_line[disc_right], (int)'#');
  TEST_ASSERT_INT_EQ((int)right_line[disc_left], (int)':');
  TEST_ASSERT_INT_EQ((int)left_line[disc_left], (int)'#');
  TEST_ASSERT_INT_EQ((int)left_line[disc_right], (int)':');
}

// Vérifie la fenêtre locale pour Paris le 14 novembre 2026 (DST standard).
static void test_web_window_paris_november_14(void) {
  // Given
  const double expected_rise_jd = JD_2026_11_14 + 11.862222 / 24.0;
  const double expected_set_jd = JD_2026_11_14 + 19.898333 / 24.0;

  // When
  double rise_jd = 0.0;
  double set_jd = 0.0;
  const double duration_hours = moon_web_window(
      2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES,
      PARIS_OFFSET_MINUTES, &rise_jd, &set_jd);

  // Then
  TEST_ASSERT_DOUBLE_EQ(rise_jd, expected_rise_jd, 1e-4);
  TEST_ASSERT_DOUBLE_EQ(set_jd, expected_set_jd, 1e-4);
  TEST_ASSERT_DOUBLE_EQ(duration_hours, 8.0364, 0.01);
}

// Vérifie que la fenêtre locale couvre 23 h le jour du passage à l'heure d'été
// à Paris (29 mars 2026) et que la durée reste cohérente avec les événements.
static void test_web_window_paris_dst_transition(void) {
  // Given
  const double start_offset = 60.0;  // +01:00 (CET)
  const double end_offset = 120.0;   // +02:00 (CEST)
  const double expected_window_hours = 23.0;

  // When
  double rise_jd = 0.0;
  double set_jd = 0.0;
  const double duration_hours =
      moon_web_window(2026, 3, 29, PARIS_LATITUDE, PARIS_LONGITUDE,
                      start_offset, end_offset, &rise_jd, &set_jd);

  const double day_start_jd = moon_web_julian_day(2026, 3, 29, 0, start_offset);
  const double day_end_jd = moon_web_julian_day(2026, 3, 29, 24, end_offset);

  // Then
  TEST_ASSERT_TRUE(isfinite(duration_hours));
  TEST_ASSERT_TRUE(duration_hours > 0.0);
  TEST_ASSERT_DOUBLE_EQ(duration_hours, (set_jd - rise_jd) * 24.0, 1e-9);
  TEST_ASSERT_DOUBLE_EQ(day_end_jd - day_start_jd, expected_window_hours / 24.0,
                        1e-9);
  TEST_ASSERT_TRUE(rise_jd >= day_start_jd - 1.0);
  TEST_ASSERT_TRUE(rise_jd <= day_end_jd + 1.0);
  TEST_ASSERT_TRUE(set_jd >= day_start_jd - 1.0);
  TEST_ASSERT_TRUE(set_jd <= day_end_jd + 1.0);
}

// Vérifie que la fenêtre locale de Sydney reste cohérente avec un décalage
// horaire important (14 novembre 2026, AEDT +11:00).
static void test_web_window_sydney_dst(void) {
  // Given
  const double sydney_lat = -33.8688;
  const double sydney_lon = 151.2093;
  const double sydney_offset = 660.0;  // +11:00 (AEDT)

  // When
  double rise_jd = 0.0;
  double set_jd = 0.0;
  const double duration_hours =
      moon_web_window(2026, 11, 14, sydney_lat, sydney_lon, sydney_offset,
                      sydney_offset, &rise_jd, &set_jd);

  const double day_start_jd =
      moon_web_julian_day(2026, 11, 14, 0, sydney_offset);
  const double day_end_jd =
      moon_web_julian_day(2026, 11, 14, 24, sydney_offset);

  // Then
  TEST_ASSERT_TRUE(isfinite(duration_hours));
  TEST_ASSERT_TRUE(duration_hours > 0.0);
  TEST_ASSERT_DOUBLE_EQ(duration_hours, (set_jd - rise_jd) * 24.0, 1e-9);
  TEST_ASSERT_DOUBLE_EQ(day_end_jd - day_start_jd, 1.0, 1e-9);
  TEST_ASSERT_TRUE(rise_jd >= day_start_jd - 1.0);
  TEST_ASSERT_TRUE(set_jd <= day_end_jd + 1.0);
}

// Vérifie le rejet des entrées invalides pour moon_web_window.
static void test_web_window_invalid_inputs(void) {
  // Given
  const double invalid_latitude = 91.0;
  const double invalid_offset = 2000.0;
  double rise_jd = 0.0;
  double set_jd = 0.0;

  // When
  const double invalid_latitude_duration = moon_web_window(
      2026, 11, 14, invalid_latitude, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES,
      PARIS_OFFSET_MINUTES, &rise_jd, &set_jd);
  const double invalid_offset_duration =
      moon_web_window(2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE,
                      invalid_offset, PARIS_OFFSET_MINUTES, &rise_jd, &set_jd);
  const double null_rise = moon_web_window(
      2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES,
      PARIS_OFFSET_MINUTES, NULL, &set_jd);
  const double null_set = moon_web_window(2026, 11, 14, PARIS_LATITUDE,
                                          PARIS_LONGITUDE, PARIS_OFFSET_MINUTES,
                                          PARIS_OFFSET_MINUTES, &rise_jd, NULL);

  // Then
  TEST_ASSERT_TRUE(isnan(invalid_latitude_duration));
  TEST_ASSERT_TRUE(isnan(invalid_offset_duration));
  TEST_ASSERT_TRUE(isnan(null_rise));
  TEST_ASSERT_TRUE(isnan(null_set));
}

// Vérifie que l'API web refuse un décalage horaire non fini : la valeur
// indéterminée doit rester un NaN et non devenir 0.
static void test_web_rejects_non_finite_offset(void) {
  // Given
  const double invalid_offset = NAN;

  // When
  const double julian_day =
      moon_web_julian_day(2026, 11, 14, 0, invalid_offset);
  const double phase = moon_web_phase(2026, 11, 14, invalid_offset);
  const double illumination =
      moon_web_illumination(2026, 11, 14, invalid_offset);
  const double duration = moon_web_duration_hours(
      2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, invalid_offset);

  // Then
  TEST_ASSERT_TRUE(isnan(julian_day));
  TEST_ASSERT_TRUE(isnan(phase));
  TEST_ASSERT_TRUE(isnan(illumination));
  TEST_ASSERT_TRUE(isnan(duration));
}

// Vérifie qu'une Lune sans lever ni coucher retourne NAN pour moon_web_window.
static void test_web_window_polar_moon_returns_nan(void) {
  // Given
  const double polar_latitude = 89.0;
  const double polar_longitude = 0.0;
  const double polar_offset = 0.0;
  double rise_jd = 0.0;
  double set_jd = 0.0;

  // When
  const double duration =
      moon_web_window(2026, 11, 14, polar_latitude, polar_longitude,
                      polar_offset, polar_offset, &rise_jd, &set_jd);

  // Then
  TEST_ASSERT_TRUE(isnan(duration));
}

// Vérifie que moon_web_duration_mode distingue une journée à deux événements.
static void test_web_duration_mode_reports_regular_day(void) {
  // Given
  const double latitude = PARIS_LATITUDE;
  const double longitude = PARIS_LONGITUDE;
  const double offset = PARIS_OFFSET_MINUTES;

  // When
  const int mode =
      moon_web_duration_mode(2026, 11, 14, latitude, longitude, offset, offset);

  // Then
  TEST_ASSERT_INT_EQ(mode, 0);
}

// Vérifie que moon_web_duration_mode signale une Lune visible toute la journée.
static void test_web_duration_mode_reports_all_day(void) {
  // Given
  const double latitude = 89.0;
  const double longitude = 0.0;
  const double offset = 0.0;

  // When
  const int mode =
      moon_web_duration_mode(2026, 11, 24, latitude, longitude, offset, offset);

  // Then
  TEST_ASSERT_INT_EQ(mode, 1);
}

// Vérifie que moon_web_duration_mode signale une Lune absente toute la journée.
static void test_web_duration_mode_reports_absent(void) {
  // Given
  const double latitude = 89.0;
  const double longitude = 0.0;
  const double offset = 0.0;

  // When
  const int mode =
      moon_web_duration_mode(2026, 11, 14, latitude, longitude, offset, offset);

  // Then
  TEST_ASSERT_INT_EQ(mode, 2);
}

// Vérifie que moon_web_duration_mode classe jour entier lorsqu'un événement
// isolé existe hors de la fenêtre journalière.
static void test_web_duration_mode_reports_all_day_with_out_of_window_event(
    void) {
  // Given
  const double latitude = 89.0;
  const double longitude = 0.0;
  const double offset = 0.0;

  // When
  const int mode =
      moon_web_duration_mode(2026, 1, 7, latitude, longitude, offset, offset);

  // Then
  TEST_ASSERT_INT_EQ(mode, 1);
}

// Vérifie que moon_web_duration_mode classe Lune absente lorsqu'un événement
// isolé existe hors de la fenêtre journalière.
static void test_web_duration_mode_reports_absent_with_out_of_window_event(
    void) {
  // Given
  const double latitude = 89.0;
  const double longitude = 0.0;
  const double offset = 0.0;

  // When
  const int mode =
      moon_web_duration_mode(2026, 1, 10, latitude, longitude, offset, offset);

  // Then
  TEST_ASSERT_INT_EQ(mode, 2);
}

// Vérifie que moon_web_duration_mode accepte des décalages de début et de fin
// distincts, comme lors d'un changement d'heure.
static void test_web_duration_mode_accepts_distinct_offsets(void) {
  // Given
  const double latitude = 89.0;
  const double longitude = 0.0;
  const double start_offset = 0.0;
  const double end_offset = -60.0;

  // When
  const int mode = moon_web_duration_mode(2026, 11, 24, latitude, longitude,
                                          start_offset, end_offset);

  // Then
  TEST_ASSERT_INT_EQ(mode, 1);
}

// Vérifie que moon_web_duration_mode refuse une entrée invalide.
static void test_web_duration_mode_rejects_invalid_inputs(void) {
  // Given
  const double latitude = 48.8566;
  const double longitude = 2.3522;
  const double offset = 60.0;

  // When
  const int invalid_date =
      moon_web_duration_mode(1899, 11, 14, latitude, longitude, offset, offset);
  const int invalid_latitude =
      moon_web_duration_mode(2026, 11, 14, 91.0, longitude, offset, offset);
  const int invalid_start_offset =
      moon_web_duration_mode(2026, 11, 14, latitude, longitude, NAN, offset);
  const int invalid_end_offset =
      moon_web_duration_mode(2026, 11, 14, latitude, longitude, offset, NAN);

  // Then
  TEST_ASSERT_INT_EQ(invalid_date, -1);
  TEST_ASSERT_INT_EQ(invalid_latitude, -1);
  TEST_ASSERT_INT_EQ(invalid_start_offset, -1);
  TEST_ASSERT_INT_EQ(invalid_end_offset, -1);
}

// Vérifie que l'aperçu à midi et l'aperçu par défaut coïncident.
static void test_web_phase_at_noon_matches_phase(void) {
  // Given
  const double expected = moon_web_phase(2026, 11, 14, PARIS_OFFSET_MINUTES);

  // When
  const double actual =
      moon_web_phase_at(2026, 11, 14, 12, 0, PARIS_OFFSET_MINUTES);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual, expected, 1e-12);
}

// Vérifie que l'aperçu à midi et l'aperçu par défaut coïncident pour
// l'inclinaison du limbe.
static void test_web_bright_limb_angle_at_noon_matches_noon(void) {
  // Given
  const double expected = moon_web_bright_limb_angle(
      2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);

  // When
  const double actual =
      moon_web_bright_limb_angle_at(2026, 11, 14, 12, 0, PARIS_LATITUDE,
                                    PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual, expected, 1e-12);
}

// Vérifie que l'aperçu à 20:30 suit l'heure locale demandée
// (19:30 UTC pour Paris).
static void test_web_preview_at_evening_hour(void) {
  // Given
  const double expected_illumination = 0.2557;
  const double expected_angle = 229.8;
  const double illumination_tolerance = 0.005;
  const double angle_tolerance = 0.05;

  // When
  const double actual_illumination =
      moon_web_illumination_at(2026, 11, 14, 20, 30, PARIS_OFFSET_MINUTES);
  const double actual_angle =
      moon_web_bright_limb_angle_at(2026, 11, 14, 20, 30, PARIS_LATITUDE,
                                    PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);

  // Then
  TEST_ASSERT_DOUBLE_EQ(actual_illumination, expected_illumination,
                        illumination_tolerance);
  TEST_ASSERT_DOUBLE_EQ(actual_angle, expected_angle, angle_tolerance);
}

// Vérifie le rejet des heures invalides pour les aperçus.
static void test_web_preview_rejects_invalid_time(void) {
  // Given
  const int invalid_minutes = 60;
  const int invalid_hours = 25;
  const int invalid_hour_negative = -1;
  const int invalid_minutes_negative = -1;

  // When
  const double phase_minutes = moon_web_phase_at(
      2026, 11, 14, 20, invalid_minutes, PARIS_OFFSET_MINUTES);
  const double phase_hours =
      moon_web_phase_at(2026, 11, 14, invalid_hours, 0, PARIS_OFFSET_MINUTES);
  const double phase_negative =
      moon_web_phase_at(2026, 11, 14, invalid_hour_negative,
                        invalid_minutes_negative, PARIS_OFFSET_MINUTES);
  const double illumination_minutes = moon_web_illumination_at(
      2026, 11, 14, 20, invalid_minutes, PARIS_OFFSET_MINUTES);
  const double angle_hours = moon_web_bright_limb_angle_at(
      2026, 11, 14, invalid_hours, 0, PARIS_LATITUDE, PARIS_LONGITUDE,
      PARIS_OFFSET_MINUTES);
  const double angle_negative = moon_web_bright_limb_angle_at(
      2026, 11, 14, invalid_hour_negative, invalid_minutes_negative,
      PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);

  // Then
  TEST_ASSERT_TRUE(isnan(phase_minutes));
  TEST_ASSERT_TRUE(isnan(phase_hours));
  TEST_ASSERT_TRUE(isnan(phase_negative));
  TEST_ASSERT_TRUE(isnan(illumination_minutes));
  TEST_ASSERT_TRUE(isnan(angle_hours));
  TEST_ASSERT_TRUE(isnan(angle_negative));
}

// Vérifie le rejet des dates invalides pour les aperçus.
static void test_web_preview_rejects_invalid_date(void) {
  // Given
  const int invalid_month = 13;
  const int invalid_day = 32;

  // When
  const double phase_month =
      moon_web_phase_at(2026, invalid_month, 14, 20, 30, PARIS_OFFSET_MINUTES);
  const double illumination_day = moon_web_illumination_at(
      2026, 11, invalid_day, 20, 30, PARIS_OFFSET_MINUTES);
  const double angle_date = moon_web_bright_limb_angle_at(
      2026, invalid_month, invalid_day, 20, 30, PARIS_LATITUDE, PARIS_LONGITUDE,
      PARIS_OFFSET_MINUTES);

  // Then
  TEST_ASSERT_TRUE(isnan(phase_month));
  TEST_ASSERT_TRUE(isnan(illumination_day));
  TEST_ASSERT_TRUE(isnan(angle_date));
}

static const TestCase test_cases[] = {
    {"test_web_ascii_size_and_capacity", test_web_ascii_size_and_capacity},
    {"test_web_julian_day_utc_midnight", test_web_julian_day_utc_midnight},
    {"test_web_julian_day_timezone_offset",
     test_web_julian_day_timezone_offset},
    {"test_web_duration_paris_november_14",
     test_web_duration_paris_november_14},
    {"test_web_event_julian_day_paris_november_14",
     test_web_event_julian_day_paris_november_14},
    {"test_web_phase_and_illumination_paris_november_14",
     test_web_phase_and_illumination_paris_november_14},
    {"test_web_altitude_at_rise", test_web_altitude_at_rise},
    {"test_web_bright_limb_angle_at_noon", test_web_bright_limb_angle_at_noon},
    {"test_web_bright_limb_angle_is_nan_at_full_moon",
     test_web_bright_limb_angle_is_nan_at_full_moon},
    {"test_web_bright_limb_angle_rejects_invalid_inputs",
     test_web_bright_limb_angle_rejects_invalid_inputs},
    {"test_web_phase_at_noon_matches_phase",
     test_web_phase_at_noon_matches_phase},
    {"test_web_bright_limb_angle_at_noon_matches_noon",
     test_web_bright_limb_angle_at_noon_matches_noon},
    {"test_web_preview_at_evening_hour", test_web_preview_at_evening_hour},
    {"test_web_preview_rejects_invalid_time",
     test_web_preview_rejects_invalid_time},
    {"test_web_preview_rejects_invalid_date",
     test_web_preview_rejects_invalid_date},
    {"test_web_polar_moon_returns_nan", test_web_polar_moon_returns_nan},
    {"test_web_phase_and_illumination_reject_invalid_date",
     test_web_phase_and_illumination_reject_invalid_date},
    {"test_web_ascii_clamps_illumination", test_web_ascii_clamps_illumination},
    {"test_web_invalid_inputs", test_web_invalid_inputs},
    {"test_web_ascii", test_web_ascii},
    {"test_web_ascii_uses_bright_limb_angle",
     test_web_ascii_uses_bright_limb_angle},
    {"test_web_window_paris_november_14", test_web_window_paris_november_14},
    {"test_web_window_paris_dst_transition",
     test_web_window_paris_dst_transition},
    {"test_web_window_sydney_dst", test_web_window_sydney_dst},
    {"test_web_window_invalid_inputs", test_web_window_invalid_inputs},
    {"test_web_rejects_non_finite_offset", test_web_rejects_non_finite_offset},
    {"test_web_window_polar_moon_returns_nan",
     test_web_window_polar_moon_returns_nan},
    {"test_web_duration_mode_reports_regular_day",
     test_web_duration_mode_reports_regular_day},
    {"test_web_duration_mode_reports_all_day",
     test_web_duration_mode_reports_all_day},
    {"test_web_duration_mode_reports_absent",
     test_web_duration_mode_reports_absent},
    {"test_web_duration_mode_reports_all_day_with_out_of_window_event",
     test_web_duration_mode_reports_all_day_with_out_of_window_event},
    {"test_web_duration_mode_reports_absent_with_out_of_window_event",
     test_web_duration_mode_reports_absent_with_out_of_window_event},
    {"test_web_duration_mode_rejects_invalid_inputs",
     test_web_duration_mode_rejects_invalid_inputs},
    {"test_web_duration_mode_accepts_distinct_offsets",
     test_web_duration_mode_accepts_distinct_offsets},
};

int main(void) {
  return runner_run("web_api", test_cases, TEST_CASE_COUNT(test_cases));
}