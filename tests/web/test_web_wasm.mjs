// test_web_wasm.mjs — Tests des exports WebAssembly : jour julien, offsets,
// durée, événements, phase et ASCII.

import assert from "node:assert/strict";
import test from "node:test";
import createMoonCore from "../../web/js/moon_core.mjs";

const moon = await createMoonCore();

const PARIS_LATITUDE = 48.8566;
const PARIS_LONGITUDE = 2.3522;
const PARIS_OFFSET_MINUTES = 60;
const JD_2026_11_14 = 2461358.5;

// Rend le dessin ASCII dans un tampon fourni par l'appelant, comme la page.
function renderAscii(phase, illumination, angle) {
  const size = moon._moon_web_ascii_size();
  const pointer = moon._malloc(size);
  if (!pointer) return "";

  const length = moon._moon_web_ascii_into(
    phase, illumination, angle, pointer, size);
  const text = length >= 0 ? moon.UTF8ToString(pointer, length) : "";
  moon._free(pointer);
  return text;
}

function assertNear(actual, expected, tolerance, label) {
  assert.ok(Number.isFinite(actual),
    `${label}: expected a finite number, received ${actual}`);
  assert.ok(Math.abs(actual - expected) <= tolerance,
    `${label}: expected ${expected} +/- ${tolerance}, received ${actual}`);
}

// Vérifie le jour julien à minuit UTC.
test("le jour julien est calculé à minuit UTC", () => {
  // Given
  const expectedJulianDay = JD_2026_11_14;
  const tolerance = 1e-9;

  // When
  const actualJulianDay = moon._moon_web_julian_day(2026, 11, 14, 0, 0);

  // Then
  assertNear(actualJulianDay, expectedJulianDay, tolerance,
    "UTC midnight Julian day");
});

// Vérifie que l'offset horaire positif recule le jour julien.
test("un offset horaire positif recule le jour julien", () => {
  // Given
  const offsetMinutes = 60;
  const expectedJulianDay = JD_2026_11_14 - offsetMinutes / 1440;
  const tolerance = 1e-9;

  // When
  const actualJulianDay =
      moon._moon_web_julian_day(2026, 11, 14, 0, offsetMinutes);

  // Then
  assertNear(actualJulianDay, expectedJulianDay, tolerance,
    "positive timezone offset");
});

// Vérifie que l'offset horaire négatif avance le jour julien.
test("un offset horaire négatif avance le jour julien", () => {
  // Given
  const offsetMinutes = -300;
  const expectedJulianDay = JD_2026_11_14 + 300 / 1440;
  const tolerance = 1e-9;

  // When
  const actualJulianDay =
      moon._moon_web_julian_day(2026, 11, 14, 0, offsetMinutes);

  // Then
  assertNear(actualJulianDay, expectedJulianDay, tolerance,
    "negative timezone offset");
});

// Vérifie la durée de visibilité lunaire à Paris le 14 novembre 2026.
test("la durée de visibilité lunaire à Paris est cohérente", () => {
  // Given
  const expectedDurationHours = 8.0364;
  const tolerance = 0.01;

  // When
  const actualDurationHours = moon._moon_web_duration_hours(
    2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);

  // Then
  assertNear(actualDurationHours, expectedDurationHours, tolerance,
    "Paris moon duration");
});

// Vérifie le lever lunaire et l'altitude correspondante à Paris.
test("le lever lunaire et l'altitude à Paris sont cohérents", () => {
  // Given
  const expectedRiseJulianDay = JD_2026_11_14 + 11.862222 / 24;
  const expectedAltitude = 0.125;
  const tolerance = 1e-4;

  // When
  const actualRiseJulianDay = moon._moon_web_event_julian_day(
    2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES, 1);
  const actualAltitude = moon._moon_web_altitude(
    actualRiseJulianDay, PARIS_LATITUDE, PARIS_LONGITUDE);

  // Then
  assertNear(actualRiseJulianDay, expectedRiseJulianDay, tolerance,
    "Paris moonrise");
  assertNear(actualAltitude, expectedAltitude, 0.01,
    "altitude at moonrise");
});

// Vérifie la phase et l'illumination à Paris le 14 novembre 2026.
test("la phase et l'illumination à Paris sont cohérentes", () => {
  // Given
  const expectedPhase = 1;
  const expectedIllumination = 0.2273;
  const tolerance = 0.005;

  // When
  const actualPhase = moon._moon_web_phase(
    2026, 11, 14, PARIS_OFFSET_MINUTES);
  const actualIllumination = moon._moon_web_illumination(
    2026, 11, 14, PARIS_OFFSET_MINUTES);

  // Then
  assert.equal(Math.round(actualPhase), expectedPhase,
    "Paris moon phase");
  assertNear(actualIllumination, expectedIllumination, tolerance,
    "Paris moon illumination");
});

// Vérifie le rejet des entrées invalides.
test("les entrées invalides sont rejetées", () => {
  // Given
  const invalidMonth = 2;
  const invalidDay = 30;
  const invalidLatitude = 91;
  const invalidEvent = 2;

  // When
  const invalidCalendar = moon._moon_web_julian_day(
    2026, invalidMonth, invalidDay, 0, 0);
  const invalidLatitudeDuration = moon._moon_web_duration_hours(
    2026, 11, 14, invalidLatitude, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);
  const invalidEventJulianDay = moon._moon_web_event_julian_day(
    2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES,
    invalidEvent);

  // Then
  assert.ok(Number.isNaN(invalidCalendar),
    "invalid calendar date is rejected");
  assert.ok(Number.isNaN(invalidLatitudeDuration),
    "invalid latitude is rejected");
  assert.ok(Number.isNaN(invalidEventJulianDay),
    "invalid event type is rejected");
});


// Vérifie l'inclinaison du limbe à Paris le 14 novembre 2026.
test("l'inclinaison du limbe à Paris est exposée", () => {
  // Given
  const expectedInclination = 302.25;
  const tolerance = 0.05;

  // When
  const actualInclination = moon._moon_web_bright_limb_angle(
    2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);

  // Then
  assertNear(actualInclination, expectedInclination, tolerance,
    "Paris bright limb inclination");
});

// Vérifie que l'inclinaison est indéterminée à la pleine lune.
test("l'inclinaison du limbe est indéterminée à la pleine lune", () => {
  // Given
  const fullMoonDay = 25;

  // When
  const actualInclination = moon._moon_web_bright_limb_angle(
    2026, 11, fullMoonDay, PARIS_LATITUDE, PARIS_LONGITUDE,
    PARIS_OFFSET_MINUTES);

  // Then
  assert.ok(Number.isNaN(actualInclination),
    "full moon bright limb inclination is undefined");
});

// Vérifie le rendu ASCII de la pleine lune.
test("le rendu ASCII de la pleine lune contient des cellules pleines", () => {
  // Given
  const phase = 4;
  const illumination = 1.0;

  // When
  const ascii = renderAscii(phase, illumination, NaN);

  // Then
  assert.ok(ascii.includes("#"),
    "full moon ASCII drawing contains full cells");
  assert.ok(ascii.includes("\n"),
    "full moon ASCII drawing contains line breaks");
});


// Vérifie que l'angle du limbe oriente le rendu ASCII.
test("l'angle du limbe oriente le rendu ASCII", () => {
  // Given
  const phase = 1;
  const illumination = 0.25;
  const litRightAngle = 270;
  const litLeftAngle = 90;
  const middleRow = 4;
  const rowLength = 41;
  const discLeft = 10;
  const discRight = 29;

  // When
  const litRight = renderAscii(phase, illumination, litRightAngle);
  const litLeft = renderAscii(phase, illumination, litLeftAngle);

  // Then
  assert.equal(litRight[middleRow * rowLength + discRight], '#',
    "right limb is lit on the right");
  assert.equal(litRight[middleRow * rowLength + discLeft], ':',
    "right limb leaves the left in shade");
  assert.equal(litLeft[middleRow * rowLength + discLeft], '#',
    "left limb is lit on the left");
  assert.equal(litLeft[middleRow * rowLength + discRight], ':',
    "left limb leaves the right in shade");
});

// Vérifie la fenêtre locale pour Paris le 14 novembre 2026 (DST standard).
test("la fenêtre locale à Paris est correcte", () => {
  // Given
  const expectedRiseJulianDay = JD_2026_11_14 + 11.862222 / 24;
  const expectedSetJulianDay = JD_2026_11_14 + 19.898333 / 24;
  const expectedDurationHours = 8.0364;
  const tolerance = 1e-4;

  // When
  const risePtr = moon._malloc(8);
  const setPtr = moon._malloc(8);
  const actualDurationHours = moon._moon_web_window(
    2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE,
    PARIS_OFFSET_MINUTES, PARIS_OFFSET_MINUTES, risePtr, setPtr);

  let actualRiseJulianDay = NaN;
  let actualSetJulianDay = NaN;
  if (Number.isFinite(actualDurationHours)) {
    actualRiseJulianDay = moon.getValue(risePtr, 'double');
    actualSetJulianDay = moon.getValue(setPtr, 'double');
  }
  moon._free(risePtr);
  moon._free(setPtr);

  // Then
  assertNear(actualRiseJulianDay, expectedRiseJulianDay, tolerance,
    "Paris window rise");
  assertNear(actualSetJulianDay, expectedSetJulianDay, tolerance,
    "Paris window set");
  assertNear(actualDurationHours, expectedDurationHours, 0.01,
    "Paris window duration");
});

// Vérifie que la fenêtre locale couvre 23 h le jour du passage à l'heure d'été.
test("la fenêtre locale à Paris couvre 23 h lors du passage à l'heure d'été", () => {
  // Given
  const startOffset = 60;   // +01:00 (CET)
  const endOffset = 120;    // +02:00 (CEST)
  const expectedWindowHours = 23;
  const tolerance = 1e-9;

  // When
  const risePtr = moon._malloc(8);
  const setPtr = moon._malloc(8);
  const actualDurationHours = moon._moon_web_window(
    2026, 3, 29, PARIS_LATITUDE, PARIS_LONGITUDE,
    startOffset, endOffset, risePtr, setPtr);

  const actualRiseJulianDay = Number.isFinite(actualDurationHours)
    ? moon.getValue(risePtr, 'double')
    : NaN;
  const actualSetJulianDay = Number.isFinite(actualDurationHours)
    ? moon.getValue(setPtr, 'double')
    : NaN;
  moon._free(risePtr);
  moon._free(setPtr);

  const dayStartJulianDay = moon._moon_web_julian_day(2026, 3, 29, 0, startOffset);
  const dayEndJulianDay = moon._moon_web_julian_day(2026, 3, 29, 24, endOffset);

  // Then
  assert.ok(Number.isFinite(actualRiseJulianDay), "DST rise is finite");
  assert.ok(Number.isFinite(actualSetJulianDay), "DST set is finite");
  assertNear(dayEndJulianDay - dayStartJulianDay, expectedWindowHours / 24,
    tolerance, "DST window length");
  assert.ok(actualRiseJulianDay >= dayStartJulianDay - 1,
    "DST rise inside the extended window");
  assert.ok(actualSetJulianDay <= dayEndJulianDay + 1,
    "DST set inside the extended window");
});

// Vérifie le rejet des entrées invalides pour moon_web_window.
test("les entrées invalides de la fenêtre locale sont rejetées", () => {
  // Given
  const invalidLatitude = 91;
  const invalidOffset = 2000;

  // When
  const risePtr = moon._malloc(8);
  const setPtr = moon._malloc(8);
  const invalidLatitudeDuration = moon._moon_web_window(
    2026, 11, 14, invalidLatitude, PARIS_LONGITUDE,
    PARIS_OFFSET_MINUTES, PARIS_OFFSET_MINUTES, risePtr, setPtr);
  const invalidOffsetDuration = moon._moon_web_window(
    2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE,
    invalidOffset, PARIS_OFFSET_MINUTES, risePtr, setPtr);
  moon._free(risePtr);
  moon._free(setPtr);

  // Then
  assert.ok(Number.isNaN(invalidLatitudeDuration),
    "invalid latitude is rejected for window");
  assert.ok(Number.isNaN(invalidOffsetDuration),
    "invalid offset is rejected for window");
});

// Vérifie que l'aperçu à 12:00 reproduit l'aperçu par défaut.
test("l'aperçu à 12:00 reproduit l'aperçu par défaut", () => {
  // Given
  const expectedPhase = moon._moon_web_phase(2026, 11, 14, PARIS_OFFSET_MINUTES);
  const expectedInclination = moon._moon_web_bright_limb_angle(
    2026, 11, 14, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);

  // When
  const actualPhase = moon._moon_web_phase_at(
    2026, 11, 14, 12, 0, PARIS_OFFSET_MINUTES);
  const actualInclination = moon._moon_web_bright_limb_angle_at(
    2026, 11, 14, 12, 0, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);

  // Then
  assert.equal(actualPhase, expectedPhase,
    "noon phase must match the default preview");
  assertNear(actualInclination, expectedInclination, 1e-9,
    "noon inclination must match the default preview");
});

// Vérifie que l'aperçu à 20:30 suit l'heure locale demandée
// (19:30 UTC pour Paris).
test("l'aperçu à 20:30 suit l'heure locale demandée", () => {
  // Given
  const expectedIllumination = 0.2557;
  const expectedInclination = 229.8;
  const illuminationTolerance = 0.005;
  const inclinationTolerance = 0.05;

  // When
  const actualIllumination = moon._moon_web_illumination_at(
    2026, 11, 14, 20, 30, PARIS_OFFSET_MINUTES);
  const actualInclination = moon._moon_web_bright_limb_angle_at(
    2026, 11, 14, 20, 30, PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES);

  // Then
  assertNear(actualIllumination, expectedIllumination, illuminationTolerance,
    "evening illumination");
  assertNear(actualInclination, expectedInclination, inclinationTolerance,
    "evening inclination");
});

// Vérifie le rejet des heures invalides dans les aperçus.
test("les heures invalides sont rejetées dans les aperçus", () => {
  // Given
  const invalidCases = [
    [2026, 11, 14, 20, 60],
    [2026, 11, 14, 25, 0],
    [2026, 11, 14, -1, 0],
  ];

  // When
  const phaseValues = invalidCases.map(([year, month, day, hour, minute]) =>
    moon._moon_web_phase_at(year, month, day, hour, minute,
      PARIS_OFFSET_MINUTES));
  const illuminationValues = invalidCases.map(([year, month, day, hour, minute]) =>
    moon._moon_web_illumination_at(year, month, day, hour, minute,
      PARIS_OFFSET_MINUTES));
  const inclinationValues = invalidCases.map(([year, month, day, hour, minute]) =>
    moon._moon_web_bright_limb_angle_at(year, month, day, hour, minute,
      PARIS_LATITUDE, PARIS_LONGITUDE, PARIS_OFFSET_MINUTES));

  // Then
  assert.ok(phaseValues.every(Number.isNaN),
    "invalid phase must be NaN");
  assert.ok(illuminationValues.every(Number.isNaN),
    "invalid illumination must be NaN");
  assert.ok(inclinationValues.every(Number.isNaN),
    "invalid inclination must be NaN");
});

// Vérifie que le module WebAssembly expose les fonctions attendues.
test("le module WebAssembly expose les fonctions attendues", () => {
  // Given
  const expectedFunctions = [
    "_moon_web_julian_day",
    "_moon_web_duration_hours",
    "_moon_web_event_julian_day",
    "_moon_web_window",
    "_moon_web_phase",
    "_moon_web_illumination",
    "_moon_web_altitude",
    "_moon_web_bright_limb_angle",
    "_moon_web_phase_at",
    "_moon_web_illumination_at",
    "_moon_web_bright_limb_angle_at",
    "_moon_web_ascii_size",
    "_moon_web_ascii_into",
    "_malloc",
    "_free",
  ];
  const expectedRuntimeMethods = ["UTF8ToString", "getValue"];

  // When
  const missingFunctions = expectedFunctions.filter(
    (name) => typeof moon[name] !== "function");
  const missingRuntimeMethods = expectedRuntimeMethods.filter(
    (name) => typeof moon[name] !== "function");

  // Then
  assert.deepEqual(missingFunctions, [],
    "all WebAssembly functions must be exported");
  assert.deepEqual(missingRuntimeMethods, [],
    "all runtime methods must be exported");
});
