// test_format.mjs — Tests du formatage des événements, durées, angles et
// graduations.

import assert from "node:assert/strict";
import test from "node:test";
import {
  formatAngle,
  formatDuration,
  formatEventTime,
  formatIllumination,
  hourTickLabels
} from "../../web/js/format.js";

// Vérifie le formatage d'un événement le jour sélectionné.
test("un événement le jour sélectionné est formaté en heure seule", () => {
  // Given
  const julianDay = 2461358.5 + 12 / 24;
  const timezone = "Europe/Paris";
  const selectedDateKey = "2026-11-14";
  const expected = "13:00:00";

  // When
  const formatted = formatEventTime(julianDay, timezone, selectedDateKey);

  // Then
  assert.equal(formatted, expected);
});

// Vérifie le formatage d'un événement au jour suivant.
test("un événement au jour suivant affiche un décalage d'un jour", () => {
  // Given
  const julianDay = 2461358.5 + 1.0;
  const timezone = "Europe/Paris";
  const selectedDateKey = "2026-11-14";
  const expected = "01:00:00 (+1 j)";

  // When
  const formatted = formatEventTime(julianDay, timezone, selectedDateKey);

  // Then
  assert.equal(formatted, expected);
});

// Vérifie qu'un événement sans lever ni coucher affiche un tiret.
test("un événement sans lever ni coucher affiche un tiret", () => {
  // Given
  const julianDay = Number.NaN;

  // When
  const formatted = formatEventTime(julianDay, "Europe/Paris", "2026-11-14");

  // Then
  assert.equal(formatted, "—");
});

// Vérifie le formatage d'une durée en heures et minutes.
test("une durée est formatée en heures et minutes", () => {
  // Given
  const hours = 8.0364;
  const expected = "8 h 02 min";

  // When
  const formatted = formatDuration(hours);

  // Then
  assert.equal(formatted, expected);
});

// Vérifie qu'une durée nulle ou négative est bornée à zéro.
test("une durée nulle ou négative est bornée à zéro", () => {
  // Given
  const nonPositiveHours = [0, -1];

  // When
  const formattedValues = nonPositiveHours.map(formatDuration);

  // Then
  assert.deepEqual(formattedValues, ["0 h 00 min", "0 h 00 min"]);
});

// Vérifie qu'une durée non finie affiche un tiret.
test("une durée non finie affiche un tiret", () => {
  // Given
  const nonFiniteHours = Number.NaN;

  // When
  const formatted = formatDuration(nonFiniteHours);

  // Then
  assert.equal(formatted, "— h — min");
});

// Vérifie les libellés de l'axe du graphique pour une journée de 24 h.
test("les libellés de l'axe suivent l'heure locale un jour normal", () => {
  // Given
  const startOffset = 60;
  const dayStartJulianDay =
    Date.UTC(2026, 10, 14) / 86400000 + 2440587.5 - startOffset / 1440;
  const expected = ["00:00", "06:00", "12:00", "18:00", "00:00 +1 j"];

  // When
  const labels = hourTickLabels(dayStartJulianDay, "Europe/Paris", "2026-11-14", [0, 6, 12, 18, 24]);

  // Then
  assert.deepEqual(labels, expected);
});

// Vérifie que l'axe reste exact le jour du passage à l'heure d'été.
test("les libellés de l'axe tiennent compte du passage à l'heure d'été", () => {
  // Given
  const startOffset = 60;
  const dayStartJulianDay =
    Date.UTC(2026, 2, 29) / 86400000 + 2440587.5 - startOffset / 1440;
  const expected = ["00:00", "07:00", "13:00", "19:00", "01:00 +1 j"];

  // When
  const labels = hourTickLabels(dayStartJulianDay, "Europe/Paris", "2026-03-29", [0, 6, 12, 18, 24]);

  // Then
  assert.deepEqual(labels, expected);
});

// Vérifie que l'illumination est affichée en pourcentage avec une décimale.
test("l'illumination est affichée avec une décimale", () => {
  // Given
  const quarter = 0.234;
  const full = 1;
  const none = 0;
  const expectedQuarter = "23.4 %";
  const expectedFull = "100.0 %";
  const expectedNone = "0.0 %";

  // When
  const actualQuarter = formatIllumination(quarter);
  const actualFull = formatIllumination(full);
  const actualNone = formatIllumination(none);

  // Then
  assert.equal(actualQuarter, expectedQuarter);
  assert.equal(actualFull, expectedFull);
  assert.equal(actualNone, expectedNone);
});

// Vérifie qu'une illumination non finie est affichée comme indisponible.
test("une illumination non finie est affichée comme indisponible", () => {
  // Given
  const invalidValues = [NaN, Infinity, -Infinity];
  const expected = "—";

  // When
  const actual = invalidValues.map((value) => formatIllumination(value));

  // Then
  assert.deepEqual(actual, [expected, expected, expected]);
});

// Vérifie que l'inclinaison est affichée en degrés avec une décimale.
test("l'inclinaison est affichée avec une décimale", () => {
  // Given
  const waxing = 302.44;
  const waning = 97.5;
  const expectedWaxing = "302.4°";
  const expectedWaning = "97.5°";

  // When
  const actualWaxing = formatAngle(waxing);
  const actualWaning = formatAngle(waning);

  // Then
  assert.equal(actualWaxing, expectedWaxing);
  assert.equal(actualWaning, expectedWaning);
});

// Vérifie qu'un angle hors tour est ramené dans [0, 360).
test("un angle hors tour est ramené dans [0, 360)", () => {
  // Given
  const negative = -57.5;
  const beyond = 360.4;
  const expectedNegative = "302.5°";
  const expectedBeyond = "0.4°";

  // When
  const actualNegative = formatAngle(negative);
  const actualBeyond = formatAngle(beyond);

  // Then
  assert.equal(actualNegative, expectedNegative);
  assert.equal(actualBeyond, expectedBeyond);
});

// Vérifie qu'une inclinaison non finie est affichée comme indisponible.
test("une inclinaison non finie est affichée comme indisponible", () => {
  // Given
  const invalidValues = [NaN, Infinity, -Infinity];
  const expected = "—";

  // When
  const actual = invalidValues.map((value) => formatAngle(value));

  // Then
  assert.deepEqual(actual, [expected, expected, expected]);
});