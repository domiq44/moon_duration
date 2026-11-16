// values.js — Conversions de valeurs : nombres finis et normalisation de texte
// (accents, casse).

"use strict";

// Convertit une valeur en nombre fini, ou null si elle n'en est pas un.
// Number(null), Number("") et Number(" ") valent 0 : ces valeurs doivent être
// rejetées, sinon un localStorage corrompu s'affiche comme une position valide.
export function toFiniteNumber(value) {
  if (typeof value === "number") {
    return Number.isFinite(value) ? value : null;
  }
  if (typeof value === "string") {
    const trimmed = value.trim();
    if (trimmed === "") return null;
    const parsed = Number(trimmed);
    return Number.isFinite(parsed) ? parsed : null;
  }
  return null;
}

// Normalise un texte pour la recherche : sans accents et en minuscules.
export function normalizeText(text) {
  return text.normalize("NFD").replace(/[\u0300-\u036f]/g, "").toLocaleLowerCase("fr");
}