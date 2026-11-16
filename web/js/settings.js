// settings.js — Lecture, écriture et suppression des réglages mémorisés dans le
// stockage local.

"use strict";

export const SETTINGS_STORAGE_KEY = "moon-duration-settings-v1";

// Lit les réglages mémorisés ; null s'ils sont absents, vides ou illisibles.
export function readSettings(storage) {
  let raw = null;
  try {
    raw = storage.getItem(SETTINGS_STORAGE_KEY);
  } catch {
    return null;
  }
  if (!raw) return null;
  try {
    const settings = JSON.parse(raw);
    return settings && typeof settings === "object" ? settings : null;
  } catch {
    return null;
  }
}

// Enregistre les réglages ; false si le navigateur refuse l'écriture.
export function writeSettings(storage, settings) {
  try {
    storage.setItem(SETTINGS_STORAGE_KEY, JSON.stringify(settings));
    return true;
  } catch {
    return false;
  }
}

// Supprime les réglages mémorisés ; false si le navigateur refuse.
export function clearSettings(storage) {
  try {
    storage.removeItem(SETTINGS_STORAGE_KEY);
    return true;
  } catch {
    return false;
  }
}