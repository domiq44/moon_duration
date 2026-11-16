// test_theme.mjs — Tests de la résolution et de l'application du thème.

import assert from "node:assert/strict";
import test from "node:test";
import { applyTheme, resolveTheme, THEME_PREFERENCES } from "../../web/js/theme.js";

const PAPER_COLOR = "#f4f6fb";

function createThemeTargets() {
  return {
    root: { dataset: {} },
    themeSelect: { value: "system" },
    metaThemeColor: { content: "" },
    getPaperColor: () => PAPER_COLOR
  };
}

// Vérifie que le thème système suit la préférence de l'appareil.
test("le thème système suit la préférence de l'appareil", () => {
  // Given
  const preference = "system";

  // When
  const darkTheme = resolveTheme(preference, true);
  const lightTheme = resolveTheme(preference, false);

  // Then
  assert.equal(darkTheme, "dark");
  assert.equal(lightTheme, "light");
});

// Vérifie qu'un thème explicite ignore la préférence de l'appareil.
test("un thème explicite ignore la préférence de l'appareil", () => {
  // Given
  const lightPreference = "light";
  const darkPreference = "dark";

  // When
  const lightTheme = resolveTheme(lightPreference, true);
  const darkTheme = resolveTheme(darkPreference, false);

  // Then
  assert.equal(lightTheme, "light");
  assert.equal(darkTheme, "dark");
});

// Vérifie que les préférences disponibles couvrent système, clair et sombre.
test("les préférences de thème couvrent système, clair et sombre", () => {
  // Given
  const expectedPreferences = ["system", "light", "dark"];

  // When
  const preferences = THEME_PREFERENCES;

  // Then
  assert.deepEqual(preferences, expectedPreferences);
});

// Vérifie que l'application du thème met à jour le document et les contrôles.
test("l'application du thème met à jour le document et les contrôles", () => {
  // Given
  const preference = "dark";
  const targets = createThemeTargets();
  const expectedResolvedTheme = "dark";

  // When
  const resolvedTheme = applyTheme(preference, { ...targets, prefersDark: false });

  // Then
  assert.equal(resolvedTheme, expectedResolvedTheme);
  assert.equal(targets.root.dataset.theme, expectedResolvedTheme);
  assert.equal(targets.themeSelect.value, preference);
  assert.equal(targets.metaThemeColor.content, PAPER_COLOR);
});

// Vérifie que le thème système est résolu puis appliqué sans valeur explicite.
test("le thème système est résolu puis appliqué", () => {
  // Given
  const preference = "system";
  const targets = createThemeTargets();

  // When
  const resolvedTheme = applyTheme(preference, { ...targets, prefersDark: false });

  // Then
  assert.equal(resolvedTheme, "light");
  assert.equal(targets.root.dataset.theme, "light");
  assert.equal(targets.themeSelect.value, "system");
});