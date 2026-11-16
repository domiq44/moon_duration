// test_values.mjs — Tests des conversions de valeurs et de la normalisation de
// texte.

import assert from "node:assert/strict";
import test from "node:test";
import { normalizeText, toFiniteNumber } from "../../web/js/values.js";

// Vérifie qu'une valeur non numérique est refusée plutôt que convertie en 0.
test("une valeur non numérique est refusée au lieu de valoir zéro", () => {
  // Given
  const rejected = [null, undefined, "", " ", true, false, "abc", NaN, Infinity];
  const expected = null;

  // When
  const actual = rejected.map((value) => toFiniteNumber(value));

  // Then
  assert.deepEqual(actual, rejected.map(() => expected));
});

// Vérifie qu'une valeur numérique, chaîne comprise, est convertie.
test("une valeur numérique est convertie en nombre fini", () => {
  // Given
  const cases = [
    [48.8566, 48.8566],
    ["48.8566", 48.8566],
    ["-2.3522", -2.3522],
    [0, 0],
  ];

  // When
  const actual = cases.map(([value]) => toFiniteNumber(value));

  // Then
  assert.deepEqual(actual, cases.map(([, expected]) => expected));
});

// Vérifie la suppression des accents et de la casse.
test("le texte est normalisé sans accents ni casse", () => {
  // Given
  const text = "Montréal";
  const expected = "montreal";

  // When
  const normalized = normalizeText(text);

  // Then
  assert.equal(normalized, expected);
});