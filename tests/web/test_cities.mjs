// test_cities.mjs — Tests du catalogue et de la recherche de villes.

import assert from "node:assert/strict";
import test from "node:test";
import { CITIES, findCity, searchCities } from "../../web/js/cities.js";

// Vérifie qu'une ville connue est retrouvée par son nom exact.
test("une ville connue est retrouvée par son nom exact", () => {
  // Given
  const expected = {
    name: "Paris",
    latitude: 48.8566,
    longitude: 2.3522,
    timezone: "Europe/Paris"
  };

  // When
  const city = findCity("Paris");

  // Then
  assert.deepEqual(city, expected);
});

// Vérifie qu'un nom inconnu ne renvoie aucune ville.
test("un nom de ville inconnu ne renvoie aucune ville", () => {
  // Given
  const unknownNames = ["", "Atlantis", undefined];

  // When
  const actual = unknownNames.map((name) => findCity(name));

  // Then
  assert.deepEqual(actual, [null, null, null]);
});

// Vérifie que la recherche ignore les accents et la casse.
test("la recherche de ville ignore les accents et la casse", () => {
  // Given
  const query = "geneve";
  const expected = ["Genève"];

  // When
  const actual = searchCities(query).map((city) => city.name);

  // Then
  assert.deepEqual(actual, expected);
});

// Vérifie que la recherche limite le nombre de résultats.
test("la recherche de ville est limitée au nombre demandé", () => {
  // Given
  const limit = 3;

  // When
  const actual = searchCities("", limit);

  // Then
  assert.equal(actual.length, limit);
  assert.ok(CITIES.length > limit);
});