/*
 * moon_phase_name.c
 * Libellés des phases lunaires (localisés via le module i18n, défaut anglais).
 */

#include "moon_phase_name.h"

#include "i18n.h"
#include "moon_phase.h"

const char *moon_phase_name(MoonPhase phase) {
  return i18n_moon_phase_name(phase);
}