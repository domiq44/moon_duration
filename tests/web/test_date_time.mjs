// test_date_time.mjs — Tests des dates, heures et fuseaux web.

import assert from "node:assert/strict";
import test from "node:test";
import {
  formatDateKey,
  instantOffsetMinutes,
  julianDayToDate,
  localDateKey,
  localDayOffsets,
  localTimeKey,
  parseDate,
  parseTime,
  resolveTimezone,
  timezoneOffsetMinutes
} from "../../web/js/date_time.js";

// Vérifie qu'une date ISO valide est convertie en date locale à midi.
test("une date ISO valide est convertie en date locale à midi", () => {
  // Given
  const value = "2026-11-14";

  // When
  const parsed = parseDate(value);

  // Then
  assert.equal(parsed.year, 2026);
  assert.equal(parsed.month, 11);
  assert.equal(parsed.day, 14);
  assert.equal(parsed.date.getFullYear(), 2026);
  assert.equal(parsed.date.getMonth(), 10);
  assert.equal(parsed.date.getDate(), 14);
  assert.equal(parsed.date.getHours(), 12);
});

// Vérifie que les dates ISO invalides sont rejetées.
test("les dates ISO invalides sont rejetées", () => {
  // Given
  const invalidValues = ["2026-02-30", "2026-1-1", "2026-11-14T00:00", ""];

  // When
  const parsedValues = invalidValues.map(parseDate);

  // Then
  assert.deepEqual(parsedValues, [null, null, null, null]);
});

// Vérifie que la clé de date ne dépend pas du fuseau.
test("la clé de date est indépendante du fuseau", () => {
  // Given
  const date = { year: 2026, month: 11, day: 14 };
  const expected = "2026-11-14";

  // When
  const actual = formatDateKey(date);

  // Then
  assert.equal(actual, expected);
});

// Vérifie que la clé de date complète les mois et jours sur deux chiffres.
test("la clé de date complète les mois et jours sur deux chiffres", () => {
  // Given
  const date = { year: 2026, month: 1, day: 5 };
  const expected = "2026-01-05";

  // When
  const actual = formatDateKey(date);

  // Then
  assert.equal(actual, expected);
});

// Vérifie l'offset horaire pour deux fuseaux connus.
test("l'offset horaire est calculé pour deux fuseaux connus", () => {
  // Given
  const date = { year: 2026, month: 11, day: 14 };

  // When
  const parisOffset = timezoneOffsetMinutes(date, "Europe/Paris");
  const newYorkOffset = timezoneOffsetMinutes(date, "America/New_York");

  // Then
  assert.equal(parisOffset, 60);
  assert.equal(newYorkOffset, -300);
});

// Vérifie que l'offset à l'instant diffère de l'offset de midi le jour du
// passage à l'heure d'été : à 00:30 locale, Paris est encore à +01:00.
test("l'offset de l'instant précède la bascule d'été", () => {
  // Given
  const date = { year: 2026, month: 3, day: 29 };
  const time = { hour: 0, minute: 30 };

  // When
  const instantOffset = instantOffsetMinutes(date, time, "Europe/Paris");
  const noonOffset = timezoneOffsetMinutes(date, "Europe/Paris");

  // Then
  assert.equal(instantOffset, 60);
  assert.equal(noonOffset, 120);
});

// Vérifie que l'offset à l'instant suit la bascule d'hiver : à 23:30 locale,
// Paris est déjà revenu à +01:00.
test("l'offset de l'instant suit la bascule d'hiver", () => {
  // Given
  const date = { year: 2026, month: 10, day: 25 };
  const time = { hour: 23, minute: 30 };

  // When
  const instantOffset = instantOffsetMinutes(date, time, "Europe/Paris");

  // Then
  assert.equal(instantOffset, 60);
});

// Vérifie que l'offset à l'instant coïncide avec l'offset de midi hors
// changement d'heure.
test("l'offset de l'instant égale l'offset de midi hors bascule", () => {
  // Given
  const date = { year: 2026, month: 11, day: 14 };
  const time = { hour: 20, minute: 30 };

  // When
  const instantOffset = instantOffsetMinutes(date, time, "Europe/Paris");
  const noonOffset = timezoneOffsetMinutes(date, "Europe/Paris");

  // Then
  assert.equal(instantOffset, noonOffset);
});

// Vérifie que l'offset à l'instant reste correct juste après la bascule d'été.
test("l'offset de l'instant après la bascule d'été", () => {
  // Given
  const date = { year: 2026, month: 3, day: 29 };
  const time = { hour: 12, minute: 0 };

  // When
  const instantOffset = instantOffsetMinutes(date, time, "Europe/Paris");

  // Then
  assert.equal(instantOffset, 120);
});

// Vérifie qu'un fuseau sans heure d'été renvoie le même offset toute l'année.
test("l'offset d'un fuseau fixe est constant", () => {
  // Given
  const date = { year: 2026, month: 3, day: 29 };
  const time = { hour: 0, minute: 30 };

  // When
  const instantOffset = instantOffsetMinutes(date, time, "UTC");

  // Then
  assert.equal(instantOffset, 0);
});

// Vérifie la conversion d'un jour julien en date UTC.
test("un jour julien est convertie en date UTC", () => {
  // Given
  const julianDay = 2451545.0;
  const expected = Date.UTC(2000, 0, 1, 12, 0, 0);

  // When
  const date = julianDayToDate(julianDay);

  // Then
  assert.equal(date.getTime(), expected);
});

// Vérifie la clé de date locale de part et d'autre de minuit UTC.
test("la clé de date locale suit le fuseau horaire", () => {
  // Given
  const date = new Date("2026-11-14T23:30:00Z");
  const expectedParis = "2026-11-15";
  const expectedNewYork = "2026-11-14";

  // When
  const parisKey = localDateKey(date, "Europe/Paris");
  const newYorkKey = localDateKey(date, "America/New_York");

  // Then
  assert.equal(parisKey, expectedParis);
  assert.equal(newYorkKey, expectedNewYork);
});

// Vérifie que les deux bornes d'une journée ordinaire partagent le même offset.
test("une journée ordinaire a deux bornes au même offset", () => {
  // Given
  const timezone = "Europe/Paris";
  const expectedOffset = 60;

  // When
  const { startOffset, endOffset } = localDayOffsets(2026, 11, 14, timezone);

  // Then
  assert.equal(startOffset, expectedOffset);
  assert.equal(endOffset, expectedOffset);
});

// Vérifie que le passage à l'heure d'été donne deux offsets différents.
test("le passage à l'heure d'été sépare les offsets de début et de fin", () => {
  // Given
  const timezone = "Europe/Paris";
  const expectedStartOffset = 60;
  const expectedEndOffset = 120;

  // When
  const { startOffset, endOffset } = localDayOffsets(2026, 3, 29, timezone);

  // Then
  assert.equal(startOffset, expectedStartOffset);
  assert.equal(endOffset, expectedEndOffset);
});

// Vérifie que le retour à l'heure d'hiver sépare les offsets de début et de fin.
test("le retour à l'heure d'hiver sépare les offsets de début et de fin", () => {
  // Given
  const timezone = "Europe/Paris";
  const expectedStartOffset = 120;
  const expectedEndOffset = 60;

  // When
  const { startOffset, endOffset } = localDayOffsets(2026, 10, 25, timezone);

  // Then
  assert.equal(startOffset, expectedStartOffset);
  assert.equal(endOffset, expectedEndOffset);
});

// Vérifie que les deux bornes encadrent exactement la journée locale.
test("les bornes locales encadrent la journée de 23 ou 25 heures", () => {
  // Given
  const timezone = "Europe/Paris";
  const springForward = localDayOffsets(2026, 3, 29, timezone);
  const fallBack = localDayOffsets(2026, 10, 25, timezone);

  const windowHours = ({ startOffset, endOffset }) =>
    (24 * 60 - endOffset) - (0 - startOffset);

  // When
  const springForwardHours = windowHours(springForward) / 60;
  const fallBackHours = windowHours(fallBack) / 60;

  // Then
  assert.equal(springForwardHours, 23);
  assert.equal(fallBackHours, 25);
});

// Vérifie que le fuseau automatique utilise le fuseau détecté.
test("le fuseau automatique est résolu depuis le navigateur", () => {
  // Given
  const detected = "Europe/Paris";
  const expected = "Europe/Paris";

  // When
  const automatic = resolveTimezone("auto", detected);

  // Then
  assert.equal(automatic, expected);
});

// Vérifie qu'une valeur explicite ignore le fuseau détecté.
test("un fuseau explicite ignore le fuseau détecté", () => {
  // Given
  const detected = "Europe/Paris";
  const expected = "Asia/Tokyo";

  // When
  const explicit = resolveTimezone("Asia/Tokyo", detected);

  // Then
  assert.equal(explicit, expected);
});

// Vérifie qu'un fuseau détecté absent retombe sur UTC.
test("un fuseau détecté absent retombe sur UTC", () => {
  // Given
  const expected = "UTC";

  // When
  const fallback = resolveTimezone("auto", undefined);

  // Then
  assert.equal(fallback, expected);
});

// Vérifie que l'heure locale suit le fuseau demandé, au format « HH:MM ».
test("l'heure locale respecte le fuseau demandé", () => {
  // Given
  const parisTime = new Date("2026-11-14T20:30:00Z");
  const sydneyTime = new Date("2026-11-14T20:30:00Z");
  const midnightParis = new Date("2026-11-14T23:00:00Z");
  const utcTime = new Date("2026-11-14T12:00:00Z");
  const expected = {
    paris: "21:30",
    sydney: "07:30",
    midnightParis: "00:00",
    utc: "12:00",
  };

  // When
  const actual = {
    paris: localTimeKey(parisTime, "Europe/Paris"),
    sydney: localTimeKey(sydneyTime, "Australia/Sydney"),
    midnightParis: localTimeKey(midnightParis, "Europe/Paris"),
    utc: localTimeKey(utcTime, "UTC"),
  };

  // Then
  assert.deepEqual(actual, expected);
});

// Vérifie que « HH:MM » est analysé en heure et minute locales.
test("une heure HH:MM est analysée en heure et minute", () => {
  // Given
  const cases = [
    ["00:00", { hour: 0, minute: 0 }],
    ["09:05", { hour: 9, minute: 5 }],
    ["23:59", { hour: 23, minute: 59 }],
    ["12:00:45", { hour: 12, minute: 0 }],
  ];

  // When
  const actual = cases.map(([value]) => parseTime(value));

  // Then
  assert.deepEqual(actual, cases.map(([, expected]) => expected));
});

// Vérifie que les heures hors limites sont rejetées.
test("les heures invalides sont rejetées", () => {
  // Given
  const invalidValues = ["", "24:00", "12:60", "9:30", "12:0", "12-00", "ab:cd"];

  // When
  const actual = invalidValues.map((value) => parseTime(value));

  // Then
  assert.deepEqual(actual, invalidValues.map(() => null));
});