// render.js — Rendu du formulaire et des résultats : libellés de phase, statut,
// erreurs et zone d'aperçu.

"use strict";

import { formatAngle, formatDuration, formatEventTime, formatIllumination } from "./format.js?v=1";

// Libellés des huit phases, dans l'ordre de moon_phase.
export const PHASE_LABELS = [
  "Nouvelle lune",
  "Premier croissant",
  "Premier quartier",
  "Lune gibbeuse croissante",
  "Pleine lune",
  "Lune gibbeuse décroissante",
  "Dernier quartier",
  "Dernier croissant"
];

// Champ et message associés à chaque erreur de saisie.
const PREVIEW_ERRORS = {
  date: { field: "date-input", message: "Choisissez une date comprise entre 1900 et 3000." },
  time: { field: "time-input", message: "Choisissez une heure valide." },
  latitude: { field: "latitude-input", message: "La latitude doit être comprise entre −90° et 90°." },
  longitude: { field: "longitude-input", message: "La longitude doit être comprise entre −180° et 180°." }
};

export function setStatus(elements, message, error = false) {
  elements["status-message"].textContent = message;
  elements["status-message"].classList.toggle("error", error);
}

// Efface les marques d'erreur des champs ; la saisie est ensuite revalidée.
export function clearPreviewErrors(elements) {
  for (const { field } of Object.values(PREVIEW_ERRORS)) {
    elements[field].removeAttribute("aria-invalid");
  }
}

// Marque le champ en cause et affiche le message correspondant.
export function showPreviewError(elements, error) {
  const entry = PREVIEW_ERRORS[error];
  if (!entry) return;
  elements[entry.field].setAttribute("aria-invalid", "true");
  setStatus(elements, entry.message, true);
}

// Affiche l'aperçu calculé : résultats, phase, dessin et lieu.
export function renderPreview(elements, { result, date, time, locationName, timezone }) {
  const dateLabel = new Intl.DateTimeFormat("fr-FR", { dateStyle: "full" }).format(date.date);
  elements["result-date"].textContent = dateLabel;
  elements["duration-value"].textContent = formatDuration(result.durationHours);
  elements["moonrise-value"].textContent = formatEventTime(result.rise, timezone, result.dateKey);
  elements["moonset-value"].textContent = formatEventTime(result.set, timezone, result.dateKey);
  elements["illumination-value"].textContent = formatIllumination(result.illumination);
  elements["inclination-value"].textContent = formatAngle(result.inclination);
  const phaseLabel = result.phase >= 0 && result.phase < PHASE_LABELS.length
    ? PHASE_LABELS[result.phase]
    : "—";
  elements["phase-chip"].textContent = phaseLabel;
  elements["moon-phase-label"].textContent = phaseLabel;
  elements["preview-note"].textContent =
    `Phase évaluée à ${String(time.hour).padStart(2, "0")}:${String(time.minute).padStart(2, "0")} locale.`;
  elements["moon-ascii"].textContent = result.ascii;
  elements["location-display"].textContent =
    `${locationName || "Position personnalisée"} · ${timezone}`;
}