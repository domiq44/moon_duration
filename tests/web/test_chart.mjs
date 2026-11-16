// test_chart.mjs — Tests du graphique : maximum, intervalles, horizon,
// graduations et jours de 23 h/25 h.

import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import test from "node:test";
import {
  CHART_HORIZON,
  CHART_STEPS,
  drawHorizonLine,
  drawMaximumMarker,
  drawMoonChart,
  findIntervals,
  findMaximumHour
} from "../../web/js/chart.js";

// Vérifie que l'horizon du graphique reste aligné sur l'horizon du modèle C.
test("l'horizon du graphique suit le modèle lunaire", () => {
  // Given
  const header = readFileSync(
    new URL("../../include/moon_events.h", import.meta.url), "utf8");
  const match = header.match(/#define\s+MOON_HORIZON_ALTITUDE_DEG\s+([0-9.]+)/);

  // When
  const modelHorizon = match ? Number(match[1]) : NaN;

  // Then
  assert.ok(Number.isFinite(modelHorizon));
  assert.equal(CHART_HORIZON, modelHorizon);
});

const CHART_WIDTH = 400;
const CHART_HEIGHT = 200;
const CHART_COLORS = { outline: "#0b1220", curve: "#d9b45b" };
const CHART_HORIZON_COLORS = { horizon: "#ff6b6b", horizonGlow: "rgba(255, 107, 107, 0.16)" };
const MAXIMUM_FONT = "bold 12px Segoe UI, sans-serif";

function createProjection(width, height) {
  const plot = { left: 38, right: width - 9, top: 12, bottom: height - 26 };
  return {
    plot,
    x: (hour) => plot.left + hour / 24 * (plot.right - plot.left),
    y: (altitude) => plot.top + (90 - altitude) / 180 * (plot.bottom - plot.top)
  };
}

function createContextRecorder() {
  const calls = [];
  const context = {
    calls,
    beginPath() { calls.push(["beginPath"]); },
    clearRect() {},
    closePath() {},
    fill() {},
    fillRect() {},
    fillText(...args) { calls.push(["fillText", ...args, context.font, context.fillStyle]); },
    lineTo(...args) { calls.push(["lineTo", ...args]); },
    moveTo(...args) { calls.push(["moveTo", ...args]); },
    restore() {},
    save() {},
    scale() {},
    setLineDash(...args) { calls.push(["setLineDash", ...args]); },
    stroke() { calls.push(["stroke", context.strokeStyle, context.lineWidth, context.lineCap]); },
    strokeText(...args) { calls.push(["strokeText", ...args, context.strokeStyle]); }
  };
  return context;
}

// Vérifie que l'heure du maximum est trouvée au midi d'une courbe sinusoïdale.
test("le maximum d'une courbe sinusoïdale est à midi", () => {
  // Given
  const altitudeAt = (hour) => Math.sin((hour - 6) / 24 * Math.PI * 2);
  const expectedMaximumHour = 12;
  const tolerance = 1e-6;

  // When
  const maximumHour = findMaximumHour(altitudeAt);

  // Then
  assert.ok(Math.abs(maximumHour - expectedMaximumHour) <= tolerance,
    `expected ${expectedMaximumHour} +/- ${tolerance}, received ${maximumHour}`);
});

// Vérifie que le maximum est borné à 24 h pour une courbe croissante.
test("le maximum d'une courbe croissante est à 24 h", () => {
  // Given
  const altitudeAt = (hour) => hour;
  const expectedMaximumHour = 24;
  const tolerance = 1e-6;

  // When
  const maximumHour = findMaximumHour(altitudeAt);

  // Then
  assert.ok(Math.abs(maximumHour - expectedMaximumHour) <= tolerance,
    `expected ${expectedMaximumHour} +/- ${tolerance}, received ${maximumHour}`);
});

// Vérifie que le maximum est borné à 0 h pour une courbe décroissante.
test("le maximum d'une courbe décroissante est à 0 h", () => {
  // Given
  const altitudeAt = (hour) => -hour;
  const expectedMaximumHour = 0;
  const tolerance = 1e-6;

  // When
  const maximumHour = findMaximumHour(altitudeAt);

  // Then
  assert.ok(Math.abs(maximumHour - expectedMaximumHour) <= tolerance,
    `expected ${expectedMaximumHour} +/- ${tolerance}, received ${maximumHour}`);
});

// Vérifie que la recherche du maximum rejette un nombre de pas invalide.
test("la recherche du maximum rejette un nombre de pas invalide", () => {
  // Given
  const altitudeAt = () => 0;
  const invalidSteps = 0;

  // When
  const call = () => findMaximumHour(altitudeAt, invalidSteps);

  // Then
  assert.throws(call, RangeError);
});

// Vérifie que l'intervalle au-dessus de l'horizon est trouvé et affiné.
test("l'intervalle au-dessus de l'horizon est trouvé et affiné", () => {
  // Given
  const altitudeAt = (hour) => Math.sin((hour - 6) / 24 * Math.PI * 2);
  const tolerance = 0.01;

  // When
  const intervals = findIntervals(altitudeAt, CHART_HORIZON);

  // Then
  assert.equal(intervals.length, 1);
  const [start, end] = intervals[0];
  assert.ok(Math.abs(start - 6.4787) <= tolerance,
    `expected start near 6.4787 h, received ${start}`);
  assert.ok(Math.abs(end - 17.5213) <= tolerance,
    `expected end near 17.5213 h, received ${end}`);
});

// Vérifie qu'une courbe toujours sous l'horizon ne produit aucun intervalle.
test("une courbe toujours sous l'horizon ne produit aucun intervalle", () => {
  // Given
  const altitudeAt = () => -10;

  // When
  const intervals = findIntervals(altitudeAt);

  // Then
  assert.deepEqual(intervals, []);
});

// Vérifie qu'une courbe toujours au-dessus de l'horizon couvre la journée.
test("une courbe toujours au-dessus de l'horizon couvre la journée", () => {
  // Given
  const altitudeAt = () => 45;

  // When
  const intervals = findIntervals(altitudeAt);

  // Then
  assert.deepEqual(intervals, [[0, 24]]);
});

// Vérifie que la ligne d'horizon est un trait rouge pointillé sur toute la largeur.
test("la ligne d'horizon est un trait rouge pointillé", () => {
  // Given
  const horizon = 0.125;
  const projection = createProjection(CHART_WIDTH, CHART_HEIGHT);
  const expectedY = projection.y(horizon);
  const context = createContextRecorder();

  // When
  drawHorizonLine(context, projection, horizon, CHART_HORIZON_COLORS);

  // Then
  const dash = context.calls.find(([name]) => name === "setLineDash");
  const lineStart = context.calls.find(([name, x, y]) =>
    name === "moveTo" && x === projection.plot.left && y === expectedY);
  const lineEnd = context.calls.find(([name, x, y]) =>
    name === "lineTo" && x === projection.plot.right && y === expectedY);
  const glowStroke = context.calls.find(([name, color, width]) =>
    name === "stroke" && color === CHART_HORIZON_COLORS.horizonGlow && width === 5);
  const lineStroke = context.calls.find(([name, color, width]) =>
    name === "stroke" && color === CHART_HORIZON_COLORS.horizon && width === 1.5);
  assert.deepEqual(dash, ["setLineDash", [5, 5]]);
  assert.ok(lineStart, "horizon line start was not drawn");
  assert.ok(lineEnd, "horizon line end was not drawn");
  assert.ok(glowStroke, "horizon glow was not drawn");
  assert.ok(lineStroke, "horizon line was not drawn");
});

// Vérifie que le repère du maximum trace une verticale vers l'horizon et affiche M.
test("le repère du maximum trace la verticale et la lettre M", () => {
  // Given
  const altitudeAt = () => 30;
  const maximumHour = 12;
  const projection = createProjection(CHART_WIDTH, CHART_HEIGHT);
  const expectedMaximumX = projection.x(maximumHour);
  const expectedMaximumY = projection.y(30);
  const expectedHorizonY = projection.y(CHART_HORIZON);
  const expectedLabelY = expectedHorizonY + 12;
  const context = createContextRecorder();

  // When
  drawMaximumMarker(context, projection, altitudeAt, maximumHour, CHART_HORIZON, CHART_COLORS);

  // Then
  const lineStart = context.calls.find(([name, x, y]) =>
    name === "moveTo" && x === expectedMaximumX && y === expectedMaximumY);
  const lineEnd = context.calls.find(([name, x, y]) =>
    name === "lineTo" && x === expectedMaximumX && y === expectedHorizonY);
  const label = context.calls.find(([name, text]) => name === "fillText" && text === "M");
  const outlinedLabel = context.calls.find(([name, text]) => name === "strokeText" && text === "M");
  assert.ok(lineStart, "maximum line start was not drawn");
  assert.ok(lineEnd, "maximum line end was not drawn");
  assert.equal(label[2], expectedMaximumX);
  assert.equal(label[3], expectedLabelY);
  assert.equal(label[4], MAXIMUM_FONT);
  assert.equal(label.at(-1), CHART_COLORS.curve);
  assert.equal(outlinedLabel.at(-1), CHART_COLORS.outline);
});

// Vérifie que la lettre M reste visible quand le maximum touche le bord droit.
test("la lettre M du maximum est bornée au bord du graphique", () => {
  // Given
  const altitudeAt = () => 30;
  const maximumHour = 24;
  const projection = createProjection(CHART_WIDTH, CHART_HEIGHT);
  const expectedLineX = projection.x(maximumHour);
  const expectedLabelX = projection.plot.right - 6;
  const expectedHorizonY = projection.y(CHART_HORIZON);
  const context = createContextRecorder();

  // When
  drawMaximumMarker(context, projection, altitudeAt, maximumHour, CHART_HORIZON, CHART_COLORS);

  // Then
  const lineStart = context.calls.find(([name, x]) => name === "moveTo" && x === expectedLineX);
  const label = context.calls.find(([name, text]) => name === "fillText" && text === "M");
  assert.ok(lineStart, "maximum line start was not drawn");
  assert.equal(label[2], expectedLabelX);
  assert.equal(label[3], expectedHorizonY + 12);
});
// Vérifie qu'un jour de 23 h s'arrête à la 23e heure écoulée.
test("un jour de 23 h couvre 23 heures", () => {
  // Given
  const altitudeAt = () => 45;

  // When
  const intervals = findIntervals(altitudeAt, CHART_HORIZON, CHART_STEPS, 23);

  // Then
  assert.deepEqual(intervals, [[0, 23]]);
});

// Vérifie qu'un jour de 25 h s'arrête à la 25e heure écoulée.
test("un jour de 25 h couvre 25 heures", () => {
  // Given
  const altitudeAt = () => 45;

  // When
  const intervals = findIntervals(altitudeAt, CHART_HORIZON, CHART_STEPS, 25);

  // Then
  assert.deepEqual(intervals, [[0, 25]]);
});

// Vérifie que le maximum est cherché sur toute la durée d'un jour de 25 h.
test("le maximum couvre un jour de 25 h", () => {
  // Given
  const altitudeAt = (hour) => hour;

  // When
  const maximumHour = findMaximumHour(altitudeAt, CHART_STEPS, 25);

  // Then
  assert.ok(Math.abs(maximumHour - 25) < 1e-6,
            `maximum ${maximumHour} inattendu`);
});

// Vérifie que le graphique d'un jour de 23 h place la dernière graduation au bord droit.
test("un jour de 23 h place la dernière graduation au bord droit", () => {
  // Given
  const width = 400;
  const height = 200;
  const plot = { left: 38, right: width - 9 };
  const context = createContextRecorder();
  const canvas = {
    getBoundingClientRect: () => ({ width, height }),
    getContext: () => context,
    setAttribute() {}
  };
  const originalWindow = globalThis.window;
  globalThis.window = { devicePixelRatio: 1 };

  // When
  try {
    drawMoonChart(canvas, {
      altitudeAt: () => 45,
      dayStartJulianDay: 0,
      currentJulianDay: -1,
      timezone: "UTC",
      selectedDateKey: "2026-01-01",
      latitude: 0,
      longitude: 0,
      dayHours: 23
    });
  } finally {
    globalThis.window = originalWindow;
  }

  // Then
  const rightLabels = context.calls.filter(
    ([name, , x]) => name === "fillText" && x === plot.right);
  assert.equal(rightLabels.length, 1);
});
