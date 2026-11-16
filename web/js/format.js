// format.js — Formatage de l'affichage : événements, durée, illumination,
// angles et graduations du graphique.

"use strict";

import { julianDayToDate, localDateKey } from "./date_time.js?v=11";

export function formatEventTime(julianDay, timezone, selectedDateKey) {
  if (!Number.isFinite(julianDay)) return "—";
  const date = julianDayToDate(julianDay);
  const time = new Intl.DateTimeFormat("fr-FR", {
    timeZone: timezone,
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hourCycle: "h23"
  }).format(date);
  const eventDateKey = localDateKey(date, timezone);
  if (eventDateKey === selectedDateKey) return time;
  const selected = new Date(`${selectedDateKey}T12:00:00Z`);
  const event = new Date(`${eventDateKey}T12:00:00Z`);
  const dayOffset = Math.round((event.getTime() - selected.getTime()) / 86400000);
  return `${time} (${dayOffset > 0 ? "+" : ""}${dayOffset} j)`;
}

export function formatDuration(hours) {
  if (!Number.isFinite(hours)) return "— h — min";
  const minutesTotal = Math.max(0, Math.round(hours * 60));
  return `${Math.floor(minutesTotal / 60)} h ${String(minutesTotal % 60).padStart(2, "0")} min`;
}

// Retourne les libellés d'un axe de graphique débutant à minuit local.
// Les échantillons sont espacés en temps écoulé : les jours de 23 h ou 25 h
// donnent donc des heures locales non équidistantes, et le dernier repère peut
// tomber le lendemain.
export function hourTickLabels(dayStartJulianDay, timezone, selectedDateKey, hours) {
  const formatter = new Intl.DateTimeFormat("fr-FR", {
    timeZone: timezone,
    hour: "2-digit",
    minute: "2-digit",
    hourCycle: "h23"
  });
  return hours.map((hour) => {
    const date = julianDayToDate(dayStartJulianDay + hour / 24);
    const label = formatter.format(date);
    return localDateKey(date, timezone) === selectedDateKey ? label : `${label} +1 j`;
  });
}

// Formate l'illumination lunaire en pourcentage avec une décimale.
// Le CLI affiche déjà une décimale : le web suit la même convention.
export function formatIllumination(illumination) {
  if (!Number.isFinite(illumination)) return "—";
  const percent = Math.max(0, Math.min(1, illumination)) * 100;
  return `${percent.toFixed(1)} %`;
}

// Formate l'inclinaison du limbe en degrés avec une décimale.
// Le CLI affiche déjà une décimale : le web suit la même convention.
export function formatAngle(angle) {
  if (!Number.isFinite(angle)) return "—";
  const normalized = ((angle % 360) + 360) % 360;
  return `${normalized.toFixed(1)}°`;
}