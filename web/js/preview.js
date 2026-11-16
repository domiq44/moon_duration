// preview.js — Validation de la saisie et calcul de l'aperçu : durée, phase,
// illumination, orientation et ASCII.

"use strict";

import {
  formatDateKey,
  instantOffsetMinutes,
  localDayOffsets,
  parseDate,
  parseTime
} from "./date_time.js?v=12";

// Rend le dessin ASCII dans un tampon fourni par l'appelant : le noyau
// n'expose plus de tampon partagé, ce qui garde l'appel réentrant.
function renderAscii(moonCore, phase, illumination, inclination) {
  const size = moonCore._moon_web_ascii_size();
  const pointer = moonCore._malloc(size);
  if (!pointer) return "";

  const length = moonCore._moon_web_ascii_into(
    phase, illumination, inclination, pointer, size);
  const text = length >= 0 ? moonCore.UTF8ToString(pointer, length) : "";
  moonCore._free(pointer);
  return text.trimEnd();
}

export const PREVIEW_YEAR_MIN = 1900;
export const PREVIEW_YEAR_MAX = 3000;

// Analyse un nombre décimal simple : [-]chiffres[.chiffres]. Rejette les
// formes hexadécimales, binaires, exponentielles, « inf », « nan », les signes
// de tête, les espaces et les caractères résiduels.
function parseCoordinate(text) {
  const match = /^-?\d+(?:\.\d+)?$/.exec(text.trim());
  if (!match) return NaN;
  return Number(match[0]);
}

// Valide les champs du formulaire et renvoie les valeurs de l'aperçu, ou
// l'identifiant du premier champ invalide.
export function readPreview({ dateValue, timeValue, latitudeText, longitudeText }) {
  const date = parseDate(dateValue);
  if (!date || date.year < PREVIEW_YEAR_MIN || date.year > PREVIEW_YEAR_MAX) {
    return { ok: false, error: "date" };
  }

  const time = parseTime(timeValue);
  if (!time) return { ok: false, error: "time" };

  const latitude = parseCoordinate(latitudeText);
  if (!Number.isFinite(latitude) || latitude < -90 || latitude > 90) {
    return { ok: false, error: "latitude" };
  }

  const longitude = parseCoordinate(longitudeText);
  if (!Number.isFinite(longitude) || longitude < -180 || longitude > 180) {
    return { ok: false, error: "longitude" };
  }

  return { ok: true, date, time, latitude, longitude, dateKey: formatDateKey(date) };
}

// Calcule l'aperçu : fenêtre locale, phase, illumination, inclinaison et dessin.
// `nowMs` fixe l'instant « Maintenant » du graphique (injectable pour les tests).
export function computePreview(moonCore, preview, timezone, nowMs = Date.now()) {
  const { date, time, latitude, longitude } = preview;
  // Décalage UTC résolu à l'instant demandé : il diffère de l'offset de midi
  // quand la journée contient un changement d'heure.
  const offset = instantOffsetMinutes(date, time, timezone);

  // Le début et la fin de la journée locale peuvent relever de deux offsets
  // UTC différents lorsque la journée contient un changement d'heure.
  const { startOffset, endOffset } = localDayOffsets(date.year, date.month, date.day, timezone);
  // Durée écoulée de la journée locale : 23 h ou 25 h lors d'un changement
  // d'heure, 24 h sinon.
  const dayHours = 24 + (startOffset - endOffset) / 60;
  const dayStartJulianDay = moonCore._moon_web_julian_day(
    date.year, date.month, date.day, 0, startOffset);

  const risePtr = moonCore._malloc(8);
  const setPtr = moonCore._malloc(8);
  const windowHours = moonCore._moon_web_window(
    date.year, date.month, date.day, latitude, longitude,
    startOffset, endOffset, risePtr, setPtr);
  let rise = NaN;
  let set = NaN;
  let durationHours = NaN;
  let durationMode = 0;
  if (Number.isFinite(windowHours)) {
    rise = moonCore.getValue(risePtr, "double");
    set = moonCore.getValue(setPtr, "double");
    durationHours = windowHours;
  } else {
    // Pas de lever ni de coucher : la journée est soit entièrement éclairée,
    // soit entièrement sombre. L'API dédiée distingue les deux cas.
    durationMode = moonCore._moon_web_duration_mode(
      date.year, date.month, date.day, latitude, longitude, startOffset,
      endOffset);
    if (durationMode === 1) {
      durationHours = 24;
    } else if (durationMode === 2) {
      durationHours = 0;
    } else {
      durationMode = -1;
    }
  }
  moonCore._free(risePtr);
  moonCore._free(setPtr);

  const phaseValue = moonCore._moon_web_phase_at(
    date.year, date.month, date.day, time.hour, time.minute, offset);
  const phase = Number.isFinite(phaseValue) ? Math.round(phaseValue) : -1;
  const illumination = moonCore._moon_web_illumination_at(
    date.year, date.month, date.day, time.hour, time.minute, offset);
  const inclination = moonCore._moon_web_bright_limb_angle_at(
    date.year, date.month, date.day, time.hour, time.minute, latitude,
    longitude, offset);
  const ascii = phase >= 0 && Number.isFinite(illumination)
    ? renderAscii(moonCore, phase, illumination, inclination)
    : "";

  return {
    dateKey: preview.dateKey,
    dayStartJulianDay,
    dayHours,
    currentJulianDay: nowMs / 86400000 + 2440587.5,
    startOffset,
    endOffset,
    rise,
    set,
    durationHours,
    durationMode,
    phase,
    illumination,
    inclination,
    ascii
  };
}