// test_app.mjs — Tests de la page assemblée avec un DOM factice : aperçu
// parisien, réglages différés et erreurs de calcul.

import assert from "node:assert/strict";
import test from "node:test";

// Stubs DOM minimalistes : la page est pilotée comme dans un navigateur, sans
// rendu réel. Le module `app.js` est importé une seule fois pour ce fichier.
const elements = new Map();
const storage = new Map();
const context = new Proxy({}, {
  get: (target, name) => (name in target ? target[name] : () => {}),
  set: (target, name, value) => { target[name] = value; return true; }
});

function createElement(id) {
  const listeners = new Map();
  return {
    id,
    value: "",
    textContent: "",
    hidden: true,
    options: [],
    dataset: {},
    classList: { toggle() {}, add() {}, remove() {} },
    setAttribute() {},
    removeAttribute() {},
    getAttribute() { return null; },
    addEventListener(type, handler) { listeners.set(type, handler); },
    dispatch(type, event = {}) { listeners.get(type)?.(event); },
    append() {},
    replaceChildren() {},
    querySelector() { return null; },
    getBoundingClientRect() { return { width: 0, height: 0 }; },
    getContext() { return context; },
    closest() { return null; }
  };
}

function elementById(id) {
  if (!elements.has(id)) {
    const element = createElement(id);
    if (id === "date-input") element.value = "2026-11-14";
    if (id === "time-input") element.value = "20:30";
    if (id === "latitude-input") element.value = "48.8566";
    if (id === "longitude-input") element.value = "2.3522";
    elements.set(id, element);
  }
  return elements.get(id);
}

const documentListeners = new Map();
globalThis.document = {
  documentElement: { dataset: {} },
  getElementById: elementById,
  querySelector: () => null,
  createElement: () => createElement("created"),
  addEventListener(type, handler) { documentListeners.set(type, handler); }
};
const windowListeners = new Map();
globalThis.window = {
  devicePixelRatio: 1,
  matchMedia: () => ({ matches: false, addEventListener() {} }),
  addEventListener(type, handler) { windowListeners.set(type, handler); },
  dispatch(type, event = {}) { windowListeners.get(type)?.(event); }
};
globalThis.getComputedStyle = () => ({ getPropertyValue: () => "#ffffff" });
globalThis.localStorage = {
  getItem: (key) => storage.get(key) ?? null,
  setItem: (key, value) => { storage.set(key, value); },
  removeItem: (key) => { storage.delete(key); }
};
globalThis.ResizeObserver = class { observe() {} };

const originalSetInterval = globalThis.setInterval;
globalThis.setInterval = () => 0;
await import("../../web/js/app.js");
globalThis.setInterval = originalSetInterval;

async function waitFor(predicate, timeoutMs = 2000) {
  const start = Date.now();
  while (!predicate()) {
    if (Date.now() - start > timeoutMs) throw new Error("délai dépassé");
    await new Promise((resolve) => setTimeout(resolve, 10));
  }
}

// Vérifie que la page calcule et affiche l'aperçu parisien du 14 novembre 2026.
test("la page calcule et affiche l'aperçu parisien", async () => {
  // Given
  // Le fuseau est fixé explicitement : le fuseau détecté dépend de la machine
  // (Europe/Paris en local, UTC sur le CI) et décalerait l'aperçu d'une heure.
  elementById("timezone-select").value = "Europe/Paris";
  const expected = {
    duration: "8 h 02 min",
    phase: "Premier croissant",
    illumination: "25.6 %",
    inclination: "229.8°",
    note: "Phase évaluée à 20:30 locale."
  };

  // When
  elementById("date-input").value = "2026-11-14";
  elementById("time-input").value = "20:30";
  elementById("latitude-input").value = "48.8566";
  elementById("longitude-input").value = "2.3522";
  elementById("date-input").dispatch("input");

  // Then
  await waitFor(() => elementById("duration-value").textContent === expected.duration);
  assert.equal(elementById("duration-value").textContent, expected.duration);
  assert.equal(elementById("phase-chip").textContent, expected.phase);
  assert.equal(elementById("illumination-value").textContent, expected.illumination);
  assert.equal(elementById("inclination-value").textContent, expected.inclination);
  assert.equal(elementById("preview-note").textContent, expected.note);
});

// Vérifie que les réglages ne sont écrits qu'après une accalmie de saisie.
test("la page diffère l'écriture des réglages", async () => {
  // Given
  storage.clear();
  const latitude = elementById("latitude-input");

  // When
  latitude.value = "10.0000";
  latitude.dispatch("input");
  const immediate = storage.get("moon-duration-settings-v1") ?? null;

  // Then
  // Aucune écriture ne doit avoir eu lieu dans le même tour d'événement.
  assert.equal(immediate, null);
  await waitFor(() => storage.has("moon-duration-settings-v1"));
  const saved = JSON.parse(storage.get("moon-duration-settings-v1"));
  assert.equal(Number(saved.latitude), 10);
});

// Vérifie que l'événement pagehide force l'écriture des réglages en attente.
test("la page vide le tampon de réglages en quittant", async () => {
  // Given
  storage.clear();
  const longitude = elementById("longitude-input");

  // When
  longitude.value = "5.0000";
  longitude.dispatch("input");
  window.dispatch("pagehide");

  // Then
  const raw = storage.get("moon-duration-settings-v1");
  assert.ok(raw);
  const saved = JSON.parse(raw);
  assert.equal(Number(saved.longitude), 5);
});

// Vérifie qu'une erreur inattendue du rendu est signalée sans interrompre la page.
test("la page signale une erreur de calcul", () => {
  // Given
  const chart = elementById("moon-chart");
  const originalGetContext = chart.getContext;
  const originalGetBoundingClientRect = chart.getBoundingClientRect;
  const originalConsoleError = console.error;
  const status = elementById("status-message");
  chart.getBoundingClientRect = () => ({ width: 320, height: 160 });
  chart.getContext = () => { throw new Error("contexte indisponible"); };
  console.error = () => {};
  status.textContent = "";

  // When
  try {
    elementById("date-input").dispatch("input");
  } finally {
    chart.getContext = originalGetContext;
    chart.getBoundingClientRect = originalGetBoundingClientRect;
    console.error = originalConsoleError;
  }

  // Then
  assert.equal(status.textContent, "Le calcul de l’aperçu a échoué.");
});