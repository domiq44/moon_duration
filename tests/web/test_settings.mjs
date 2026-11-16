// test_settings.mjs — Tests de la lecture, de l'écriture et de la suppression
// des réglages.

import assert from "node:assert/strict";
import test from "node:test";
import {
  SETTINGS_STORAGE_KEY,
  clearSettings,
  readSettings,
  writeSettings
} from "../../web/js/settings.js";

function createStorage(initial = {}) {
  const values = new Map(Object.entries(initial));
  return {
    getItem: (key) => values.get(key) ?? null,
    setItem: (key, value) => { values.set(key, value); },
    removeItem: (key) => { values.delete(key); }
  };
}

function createBrokenStorage() {
  return {
    getItem() { throw new Error("stockage refusé"); },
    setItem() { throw new Error("stockage refusé"); },
    removeItem() { throw new Error("stockage refusé"); }
  };
}

// Vérifie qu'un réglage absent, vide ou illisible est ignoré.
test("des réglages absents, vides ou illisibles sont ignorés", () => {
  // Given
  const empty = createStorage();
  const corrupted = createStorage({ [SETTINGS_STORAGE_KEY]: "{incorrect" });
  const notAnObject = createStorage({ [SETTINGS_STORAGE_KEY]: "42" });
  const broken = createBrokenStorage();

  // When
  const actual = [
    readSettings(empty),
    readSettings(corrupted),
    readSettings(notAnObject),
    readSettings(broken)
  ];

  // Then
  assert.deepEqual(actual, [null, null, null, null]);
});

// Vérifie que les réglages sont enregistrés puis relus à l'identique.
test("les réglages sont enregistrés puis relus à l'identique", () => {
  // Given
  const storage = createStorage();
  const settings = {
    city: "Paris",
    latitude: "48.8566",
    longitude: "2.3522",
    timezone: "Europe/Paris",
    theme: "dark"
  };

  // When
  const saved = writeSettings(storage, settings);
  const restored = readSettings(storage);

  // Then
  assert.equal(saved, true);
  assert.deepEqual(restored, settings);
});

// Vérifie que les réglages mémorisés peuvent être supprimés.
test("les réglages mémorisés sont supprimés", () => {
  // Given
  const storage = createStorage({ [SETTINGS_STORAGE_KEY]: "{}" });

  // When
  const cleared = clearSettings(storage);
  const restored = readSettings(storage);

  // Then
  assert.equal(cleared, true);
  assert.equal(restored, null);
});

// Vérifie qu'un stockage refusé par le navigateur est signalé.
test("un stockage refusé par le navigateur est signalé", () => {
  // Given
  const storage = createBrokenStorage();

  // When
  const saved = writeSettings(storage, {});
  const cleared = clearSettings(storage);

  // Then
  assert.deepEqual([saved, cleared], [false, false]);
});