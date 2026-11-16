// theme.js — Résolution et application du thème clair/sombre et de la couleur
// de l'interface.

export const THEME_PREFERENCES = ["system", "light", "dark"];

export function resolveTheme(preference, prefersDark) {
  if (preference === "light" || preference === "dark") return preference;
  return prefersDark ? "dark" : "light";
}

export function applyTheme(preference, { root, themeSelect, metaThemeColor, prefersDark, getPaperColor }) {
  const resolved = resolveTheme(preference, prefersDark);
  root.dataset.theme = resolved;
  if (themeSelect) themeSelect.value = preference;
  if (metaThemeColor && typeof getPaperColor === "function") {
    const paper = getPaperColor();
    if (paper) metaThemeColor.content = paper;
  }
  return resolved;
}