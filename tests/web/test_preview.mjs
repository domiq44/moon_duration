// test_preview.mjs — Tests de la validation de saisie et du calcul de l'aperçu
// sur le noyau WebAssembly.

import assert from "node:assert/strict";
import test from "node:test";
import createMoonCore from "../../web/js/moon_core.mjs";
import { computePreview, readPreview } from "../../web/js/preview.js";

const moon = await createMoonCore();

const PARIS_TIMEZONE = "Europe/Paris";
const PARIS_INPUT = {
  dateValue: "2026-11-14",
  timeValue: "20:30",
  latitudeText: "48.8566",
  longitudeText: "2.3522"
};
const NOW_MS = Date.UTC(2026, 10, 14, 12, 0, 0);

function previewFrom(overrides) {
  return readPreview({ ...PARIS_INPUT, ...overrides });
}

// Vérifie qu'une saisie valide est acceptée et conservée telle quelle.
test("une saisie valide est acceptée et conservée", () => {
  // Given
  const expectedDateKey = "2026-11-14";

  // When
  const preview = previewFrom({});

  // Then
  assert.equal(preview.ok, true);
  assert.equal(preview.dateKey, expectedDateKey);
  assert.equal(preview.time.hour, 20);
  assert.equal(preview.time.minute, 30);
  assert.equal(preview.latitude, 48.8566);
  assert.equal(preview.longitude, 2.3522);
});

// Vérifie que chaque champ invalide est refusé à son tour.
test("chaque champ invalide est refusé à son tour", () => {
  // Given
  const cases = [
    [{ dateValue: "2026-02-30" }, "date"],
    [{ dateValue: "1899-12-31" }, "date"],
    [{ dateValue: "3001-01-01" }, "date"],
    [{ timeValue: "24:00" }, "time"],
    [{ latitudeText: "" }, "latitude"],
    [{ latitudeText: "91" }, "latitude"],
    [{ latitudeText: "abc" }, "latitude"],
    [{ longitudeText: "-181" }, "longitude"]
  ];

  // When
  const actual = cases.map(([overrides]) => previewFrom(overrides));

  // Then
  assert.deepEqual(actual, cases.map(([, error]) => ({ ok: false, error })));
});

// Vérifie que l'aperçu parisien du 14 novembre 2026 à 20:30 est calculé.
test("l'aperçu parisien à 20:30 est calculé", () => {
  // Given
  const expectedPhase = 1;
  const expectedDurationHours = 8.0364;
  const expectedIllumination = 0.2557;
  const expectedInclination = 229.8;
  const tolerance = 0.01;

  // When
  const result = computePreview(moon, previewFrom({}), PARIS_TIMEZONE, NOW_MS);

  // Then
  assert.equal(result.phase, expectedPhase);
  assert.ok(Math.abs(result.durationHours - expectedDurationHours) <= tolerance,
    `expected ${expectedDurationHours} h, received ${result.durationHours}`);
  assert.ok(Math.abs(result.illumination - expectedIllumination) <= 0.005,
    `expected ${expectedIllumination}, received ${result.illumination}`);
  assert.ok(Math.abs(result.inclination - expectedInclination) <= 0.05,
    `expected ${expectedInclination}°, received ${result.inclination}`);
});

// Vérifie que l'aperçu expose la fenêtre locale et le dessin ASCII.
test("l'aperçu expose la fenêtre locale et le dessin ASCII", () => {
  // Given
  const dayStartJulianDay =
    Date.UTC(2026, 10, 14) / 86400000 + 2440587.5 - 60 / 1440;

  // When
  const result = computePreview(moon, previewFrom({}), PARIS_TIMEZONE, NOW_MS);

  // Then
  assert.ok(Number.isFinite(result.rise));
  assert.ok(Number.isFinite(result.set));
  assert.ok(Math.abs(result.dayStartJulianDay - dayStartJulianDay) < 1e-9,
    `expected ${dayStartJulianDay}, received ${result.dayStartJulianDay}`);
  assert.equal(result.ascii.split("\n").length > 1, true);
});

// Vérifie que l'instant « Maintenant » est déduit de l'horloge fournie.
test("l'instant Maintenant est déduit de l'horloge fournie", () => {
  // Given
  const expectedJulianDay = NOW_MS / 86400000 + 2440587.5;

  // When
  const result = computePreview(moon, previewFrom({}), PARIS_TIMEZONE, NOW_MS);

  // Then
  assert.equal(result.currentJulianDay, expectedJulianDay);
});

// Vérifie qu'une journée à deux événements conserve le mode lever et coucher.
test("une journée à deux événements expose le mode lever et coucher", () => {
  // Given
  const expectedMode = 0;

  // When
  const result = computePreview(moon, previewFrom({}), PARIS_TIMEZONE, NOW_MS);

  // Then
  assert.equal(result.durationMode, expectedMode);
  assert.ok(Number.isFinite(result.durationHours));
});

// Vérifie qu'une Lune visible toute la journée donne vingt-quatre heures.
test("une Lune visible toute la journée donne vingt-quatre heures", () => {
  // Given
  const polarDay = previewFrom({
    dateValue: "2026-11-24",
    latitudeText: "89",
    longitudeText: "0"
  });

  // When
  const result = computePreview(moon, polarDay, "UTC", NOW_MS);

  // Then
  assert.equal(result.durationMode, 1);
  assert.equal(result.durationHours, 24);
  assert.ok(Number.isNaN(result.rise));
  assert.ok(Number.isNaN(result.set));
});

// Vérifie qu'une Lune absente toute la journée donne une durée nulle.
test("une Lune absente toute la journée donne une durée nulle", () => {
  // Given
  const polarNight = previewFrom({
    dateValue: "2026-11-14",
    latitudeText: "89",
    longitudeText: "0"
  });

  // When
  const result = computePreview(moon, polarNight, "UTC", NOW_MS);

  // Then
  assert.equal(result.durationMode, 2);
  assert.equal(result.durationHours, 0);
  assert.ok(Number.isNaN(result.rise));
  assert.ok(Number.isNaN(result.set));
});

// Vérifie qu'une Lune visible toute la journée est distinguée d'un événement
// isolé situé hors de la fenêtre journalière.
test("une Lune visible toute la journée avec événement hors fenêtre donne vingt-quatre heures", () => {
  // Given
  const polarDay = previewFrom({
    dateValue: "2026-01-07",
    latitudeText: "89",
    longitudeText: "0"
  });

  // When
  const result = computePreview(moon, polarDay, "UTC", NOW_MS);

  // Then
  assert.equal(result.durationMode, 1);
  assert.equal(result.durationHours, 24);
  assert.ok(Number.isNaN(result.rise));
  assert.ok(Number.isNaN(result.set));
});

// Vérifie qu'une Lune absente toute la journée est distinguée d'un événement
// isolé situé hors de la fenêtre journalière.
test("une Lune absente toute la journée avec événement hors fenêtre donne une durée nulle", () => {
  // Given
  const polarNight = previewFrom({
    dateValue: "2026-01-10",
    latitudeText: "89",
    longitudeText: "0"
  });

  // When
  const result = computePreview(moon, polarNight, "UTC", NOW_MS);

  // Then
  assert.equal(result.durationMode, 2);
  assert.equal(result.durationHours, 0);
  assert.ok(Number.isNaN(result.rise));
  assert.ok(Number.isNaN(result.set));
});