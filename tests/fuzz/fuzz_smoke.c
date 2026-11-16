/*
 * fuzz_smoke.c
 * Fuzzing par tirages pseudo-aleatoires : aucune bibliotheque externe, mais des
 * entrees variees soumises a ASan/UBSan. Les memes graines donnent la meme
 * sequence, ce qui rend un echec reproductible.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "datetime.h"
#include "moon_duration.h"
#include "moon_events.h"
#include "moon_orientation.h"
#include "moon_phase.h"
#include "web_api.h"

#define FUZZ_DEFAULT_ITERATIONS 5000UL
#define FUZZ_DEFAULT_SEED 0x9E3779B97F4A7C15ULL

static uint64_t rng_state = FUZZ_DEFAULT_SEED;

static uint64_t rng_next(void) {
  uint64_t x = rng_state;

  x ^= x << UINT64_C(13);
  x ^= x >> UINT64_C(7);
  x ^= x << UINT64_C(17);
  rng_state = x;
  return x;
}

static int rng_range(int min, int max) {
  return min + (int)(rng_next() % (uint64_t)(max - min + 1));
}

static double rng_double(double min, double max) {
  const double unit = (double)(rng_next() >> UINT64_C(11)) / 9007199254740992.0;

  return min + (max - min) * unit;
}

// Fuzzing de l'analyse des arguments : jetons plausibles et pieges melanges.
static void fuzz_cli(void) {
  static const char *const tokens[] = {
      "--lang", "fr",    "xx",         "--lat",      "91",       "-91",
      "abc",    "--lon", "2026-11-14", "2026-13-45", "--help",   "--version",
      "",       "-",     "1e999",      "nan",        "--lat=48", "2026-02-30",
      "0",      "180.5", "2026-1-1",   "--lon=0",    "--at",     "12:00",
      "24:00",  "9:30",  "23:59",      "12:60",      "0x1p3",    "inf",
      "1e",     ".5",    "1.2.3",      "-0",         "1e-3",     "1.",
  };
  const int token_count = (int)(sizeof(tokens) / sizeof(tokens[0]));
  char *argv[8];
  CliArgs args;

  argv[0] = (char *)"moon_duration";
  const int argc = 1 + rng_range(0, 6);

  for (int i = 1; i < argc; ++i) {
    argv[i] = (char *)tokens[rng_range(0, token_count - 1)];
  }

  (void)cli_parse(argc, argv, &args);
}

// Fuzzing des dates : composantes hors bornes, decalages extremes.
static void fuzz_datetime(void) {
  DateTime datetime = datetime_make(rng_range(-5000, 5000), rng_range(-2, 14),
                                    rng_range(-2, 32), rng_range(-2, 26),
                                    rng_range(-2, 62), rng_range(-2, 62));
  const int day_shift = rng_range(-100000, 100000);

  (void)datetime_is_valid(datetime);
  (void)datetime_weekday(datetime);
  (void)datetime_to_julian(datetime);
  (void)datetime_to_julian_gregorian(datetime);

  datetime = datetime_add_days(datetime, day_shift);
  (void)datetime_is_valid(datetime);
  (void)datetime_weekday(datetime);
  (void)datetime_to_julian(datetime);

  (void)datetime_from_julian(rng_double(-1e6, 8.1e6));
}

// Fuzzing des phases : valeurs non finies et magnitudes extremes.
static void fuzz_phase(void) {
  const double specials[] = {NAN,   INFINITY, -INFINITY, 0.0,   -0.0,
                             1e308, -1e308,   2451550.1, 1e-308};
  const int special_count = (int)(sizeof(specials) / sizeof(specials[0]));

  for (int i = 0; i < special_count; ++i) {
    (void)moon_phase_from_julian_day(specials[i]);
    (void)moon_illumination(specials[i]);
  }

  for (int i = 0; i < 16; ++i) {
    const double julian_day = rng_double(-1e9, 1e9);

    (void)moon_phase_from_julian_day(julian_day);
    (void)moon_illumination(julian_day);
    (void)moon_bright_limb_angle_geocentric(julian_day);
    (void)moon_bright_limb_angle(julian_day, rng_double(-90.0, 90.0),
                                 rng_double(-180.0, 180.0));
    (void)moon_parallactic_angle(julian_day, rng_double(-90.0, 90.0),
                                 rng_double(-180.0, 180.0));
  }
}

// Fuzzing des evenements et de la duree : fenetres courtes pour rester dans le
// budget du harnais, coordonnees variees dont les poles et les valeurs non
// finies.
static void fuzz_events(void) {
  static const double special_latitudes[] = {NAN, 90.0, -90.0};
  const int special_count =
      (int)(sizeof(special_latitudes) / sizeof(special_latitudes[0]));
  MoonEvent events[8];
  MoonDuration duration;

  for (int i = 0; i < special_count + 2; ++i) {
    const double start = rng_double(-1e9, 1e9);
    const double latitude =
        (i < special_count) ? special_latitudes[i] : rng_double(-90.0, 90.0);
    const double longitude = rng_double(-180.0, 180.0);
    const double span = rng_double(0.0, 0.3);

    (void)moon_events_scan(start, start + span, latitude, longitude, events,
                           (int)(sizeof(events) / sizeof(events[0])));
    (void)moon_duration_compute(start, start + span, latitude, longitude,
                                &duration);
  }
}

// Fuzzing des exports web : entrees valides et invalides alternees.
static void fuzz_web_api(void) {
  const int year = rng_range(1800, 3100);
  const int month = rng_range(-1, 13);
  const int day = rng_range(-1, 32);
  const double latitude = rng_double(-100.0, 100.0);
  const double longitude = rng_double(-200.0, 200.0);
  const double offset = rng_double(-2000.0, 2000.0);
  double rise_julian_day = 0.0;
  double set_julian_day = 0.0;

  (void)moon_web_julian_day(year, month, day, rng_range(-1, 25), offset);
  (void)moon_web_duration_hours(year, month, day, latitude, longitude, offset);
  (void)moon_web_event_julian_day(year, month, day, latitude, longitude, offset,
                                  rng_range(-1, 2));
  (void)moon_web_window(year, month, day, latitude, longitude, offset, offset,
                        &rise_julian_day, &set_julian_day);
  (void)moon_web_phase(year, month, day, offset);
  (void)moon_web_illumination(year, month, day, offset);
  (void)moon_web_altitude(rng_double(-1e9, 1e9), latitude, longitude);
  (void)moon_web_bright_limb_angle(year, month, day, latitude, longitude,
                                   offset);

  const int phase = rng_range(-2, 9);
  char ascii_buffer[512];
  const int ascii_length = moon_web_ascii_into(
      phase, rng_double(-1.0, 2.0), rng_double(-720.0, 720.0), ascii_buffer,
      (int)sizeof(ascii_buffer));

  (void)ascii_length;
}

int main(int argc, char **argv) {
  unsigned long iterations = FUZZ_DEFAULT_ITERATIONS;
  const char *seed_env = getenv("FUZZ_SEED");

  if (argc > 1) {
    iterations = strtoul(argv[1], NULL, 10);
  }
  if (seed_env != NULL) {
    rng_state = strtoull(seed_env, NULL, 10);
  }
  if (rng_state == 0) {
    rng_state = FUZZ_DEFAULT_SEED;
  }

  for (unsigned long i = 0; i < iterations; ++i) {
    fuzz_cli();
    fuzz_datetime();
    fuzz_phase();
    fuzz_events();
    fuzz_web_api();
  }

  printf("fuzzing : %lu iterations sans rapport de sanitizer\n", iterations);
  return 0;
}