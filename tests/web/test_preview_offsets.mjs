// test_preview_offsets.mjs — Tests des décalages et de la durée des jours 23
// h/25 h avec un noyau factice.

import assert from "node:assert/strict";
import test from "node:test";

import { computePreview } from "../../web/js/preview.js";

// Noyau factice : il enregistre les appels et fournit des résultats déterministes.
// Ce test n'a pas besoin du binaire WebAssembly.
function setupCore({ windowHours = 2 } = {}) {
  const calls = [];
  const values = new Map();
  let nextPointer = 1;
  const core = {
    calls,
    _moon_web_julian_day(year, month, day, hour, offset) {
      calls.push({ name: "julian_day", year, month, day, hour, offset });
      return Date.UTC(year, month - 1, day, hour) / 86400000 + 2440587.5 -
        offset / 1440;
    },
    _moon_web_window(year, month, day, latitude, longitude, startOffset, endOffset,
                     risePointer, setPointer) {
      calls.push({ name: "window", year, month, day, latitude, longitude,
                   startOffset, endOffset });
      values.set(risePointer, 1.0);
      values.set(setPointer, 2.0);
      return windowHours;
    },
    _moon_web_duration_mode(year, month, day, latitude, longitude, startOffset,
                            endOffset) {
      calls.push({ name: "duration_mode", year, month, day, latitude, longitude,
                   startOffset, endOffset });
      return 1;
    },
    _malloc() { return nextPointer++; },
    _free() {},
    getValue(pointer) { return values.get(pointer); },
    _moon_web_phase_at(year, month, day, hour, minute, offset) {
      calls.push({ name: "phase_at", year, month, day, hour, minute, offset });
      return 1;
    },
    _moon_web_illumination_at(year, month, day, hour, minute, offset) {
      calls.push({ name: "illumination_at", year, month, day, hour, minute, offset });
      return 0.5;
    },
    _moon_web_bright_limb_angle_at(year, month, day, hour, minute, latitude, longitude,
                                   offset) {
      calls.push({ name: "limb_angle_at", year, month, day, hour, minute, latitude,
                   longitude, offset });
      return 0;
    },
    _moon_web_ascii_size() { return 64; },
    _moon_web_ascii_into() { return 0; },
    UTF8ToString() { return ""; }
  };
  return core;
}

// Prépare un aperçu à midi pour une date donnée.
function setupPreview(year, month, day, time = { hour: 12, minute: 0 }) {
  return {
    date: { year, month, day },
    time,
    latitude: 48.8566,
    longitude: 2.3522,
    dateKey: `${year}-${String(month).padStart(2, "0")}-${String(day).padStart(2, "0")}`
  };
}

// Vérifie qu'un passage à l'heure d'été raccourcit la journée à 23 h.
test("un jour de printemps dure 23 heures", () => {
  // Given
  const core = setupCore();
  const preview = setupPreview(2026, 3, 29);

  // When
  const result = computePreview(core, preview, "Europe/Paris", 0);

  // Then
  assert.equal(result.startOffset, 60);
  assert.equal(result.endOffset, 120);
  assert.equal(result.dayHours, 23);
  const windowCall = core.calls.find((call) => call.name === "window");
  assert.equal(windowCall.startOffset, 60);
  assert.equal(windowCall.endOffset, 120);
});

// Vérifie qu'un retour à l'heure d'hiver allonge la journée à 25 h.
test("un jour d'automne dure 25 heures", () => {
  // Given
  const core = setupCore();
  const preview = setupPreview(2026, 10, 25);

  // When
  const result = computePreview(core, preview, "Europe/Paris", 0);

  // Then
  assert.equal(result.startOffset, 120);
  assert.equal(result.endOffset, 60);
  assert.equal(result.dayHours, 25);
  const windowCall = core.calls.find((call) => call.name === "window");
  assert.equal(windowCall.startOffset, 120);
  assert.equal(windowCall.endOffset, 60);
});

// Vérifie qu'une journée sans changement d'heure dure 24 h.
test("un jour ordinaire dure 24 heures", () => {
  // Given
  const core = setupCore();
  const preview = setupPreview(2026, 11, 14);

  // When
  const result = computePreview(core, preview, "Europe/Paris", 0);

  // Then
  assert.equal(result.dayHours, 24);
});

// Vérifie que le mode de durée reçoit les deux décalages de la journée.
test("le mode de durée reçoit les deux décalages", () => {
  // Given
  const core = setupCore({ windowHours: Number.NaN });
  const preview = setupPreview(2026, 10, 25);

  // When
  const result = computePreview(core, preview, "Europe/Paris", 0);

  // Then
  const modeCall = core.calls.find((call) => call.name === "duration_mode");
  assert.ok(modeCall);
  assert.equal(modeCall.startOffset, 120);
  assert.equal(modeCall.endOffset, 60);
  assert.equal(result.durationMode, 1);
  assert.equal(result.durationHours, 24);
});

// Vérifie que la phase est évaluée avec l'offset de l'instant demandé, et non
// l'offset de midi : avant la bascule d'été, 00:30 locale est encore à +01:00.
test("la phase utilise l'offset de l'instant, pas celui de midi", () => {
  // Given
  const core = setupCore();
  const preview = setupPreview(2026, 3, 29, { hour: 0, minute: 30 });

  // When
  computePreview(core, preview, "Europe/Paris", 0);

  // Then
  const phaseCall = core.calls.find((call) => call.name === "phase_at");
  const limbCall = core.calls.find((call) => call.name === "limb_angle_at");
  assert.equal(phaseCall.offset, 60);
  assert.equal(limbCall.offset, 60);
  assert.notEqual(phaseCall.offset, 120);
});

// Vérifie que l'illumination utilise elle aussi l'offset de l'instant demandé.
test("l'illumination utilise l'offset de l'instant, pas celui de midi", () => {
  // Given
  const core = setupCore();
  const preview = setupPreview(2026, 10, 25, { hour: 23, minute: 30 });

  // When
  computePreview(core, preview, "Europe/Paris", 0);

  // Then
  const call = core.calls.find((call) => call.name === "illumination_at");
  assert.equal(call.offset, 60);
  assert.notEqual(call.offset, 120);
});