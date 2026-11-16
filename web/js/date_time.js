// date_time.js — Dates, heures et fuseaux : parsing, décalages UTC, jours
// locaux de 23 h/25 h et conversions juliennes.

"use strict";

export function parseDate(value) {
  const match = /^(\d{4})-(\d{2})-(\d{2})$/.exec(value);
  if (!match) return null;
  const [, yearText, monthText, dayText] = match;
  const year = Number(yearText);
  const month = Number(monthText);
  const day = Number(dayText);
  const date = new Date(year, month - 1, day, 12);
  if (date.getFullYear() !== year || date.getMonth() !== month - 1 || date.getDate() !== day) return null;
  return { year, month, day, date };
}

// Clé de date « YYYY-MM-DD » indépendante du fuseau.
export function formatDateKey(date) {
  return `${date.year}-${String(date.month).padStart(2, "0")}-${String(date.day).padStart(2, "0")}`;
}

export function timezoneOffsetMinutes(date, timezone) {
  const instant = new Date(Date.UTC(date.year, date.month - 1, date.day, 12));
  const parts = new Intl.DateTimeFormat("en", {
    timeZone: timezone,
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hourCycle: "h23"
  }).formatToParts(instant).reduce((values, part) => {
    if (part.type !== "literal") values[part.type] = Number(part.value);
    return values;
  }, {});
  const localAsUtc = Date.UTC(parts.year, parts.month - 1, parts.day, parts.hour, parts.minute, parts.second);
  return (localAsUtc - instant.getTime()) / 60000;
}

function offsetAtInstant(instantMs, timezone) {
  const parts = new Intl.DateTimeFormat("en", {
    timeZone: timezone,
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hourCycle: "h23"
  }).formatToParts(new Date(instantMs)).reduce((values, part) => {
    if (part.type !== "literal") values[part.type] = Number(part.value);
    return values;
  }, {});
  const localAsUtc = Date.UTC(parts.year, parts.month - 1, parts.day, parts.hour, parts.minute, parts.second);
  return (localAsUtc - Math.floor(instantMs / 1000) * 1000) / 60000;
}

function localMidnightOffset(year, month, day, timezone) {
  const calendarAsUtc = Date.UTC(year, month - 1, day, 0, 0, 0);
  let instant = calendarAsUtc - offsetAtInstant(calendarAsUtc, timezone) * 60000;
  instant = calendarAsUtc - offsetAtInstant(instant, timezone) * 60000;
  return offsetAtInstant(instant, timezone);
}

// Décalage UTC (en minutes) du fuseau à un instant précis. Contrairement à
// `timezoneOffsetMinutes`, qui décrit la journée à midi, cette fonction résout
// l'heure d'été à l'instant demandé : c'est elle qu'il faut utiliser pour
// convertir une heure locale en UTC lors d'un changement d'heure.
export function instantOffsetMinutes(date, time, timezone) {
  const wallAsUtc = Date.UTC(date.year, date.month - 1, date.day, time.hour, time.minute);
  let offset = offsetAtInstant(wallAsUtc, timezone);
  const instant = wallAsUtc - offset * 60000;
  // Un second passage absorbe les cas où l'offset changé déplace l'instant
  // dans une autre plage de fuseau (bascule à minuit, heure ambiguë).
  offset = offsetAtInstant(instant, timezone);
  return offsetAtInstant(wallAsUtc - offset * 60000, timezone);
}

export function localDayOffsets(year, month, day, timezone) {
  const nextDay = new Date(Date.UTC(year, month - 1, day + 1));
  return {
    startOffset: localMidnightOffset(year, month, day, timezone),
    endOffset: localMidnightOffset(nextDay.getUTCFullYear(),
                                   nextDay.getUTCMonth() + 1,
                                   nextDay.getUTCDate(), timezone)
  };
}

export function julianDayToDate(julianDay) {
  return new Date((julianDay - 2440587.5) * 86400000);
}

export function localDateKey(date, timezone) {
  const parts = new Intl.DateTimeFormat("en-CA", {
    timeZone: timezone,
    year: "numeric",
    month: "2-digit",
    day: "2-digit"
  }).formatToParts(date).reduce((values, part) => {
    if (part.type !== "literal") values[part.type] = part.value;
    return values;
  }, {});
  return `${parts.year}-${parts.month}-${parts.day}`;
}

// Résout le fuseau sélectionné : "auto" (ou vide) utilise le fuseau détecté.
export function resolveTimezone(value, detectedTimezone) {
  if (value && value !== "auto") return value;
  return detectedTimezone || "UTC";
}

export function todayInTimezone(timezone) {
  return localDateKey(new Date(), timezone);
}

// Heure locale « HH:MM » dans le fuseau donné.
export function localTimeKey(date, timezone) {
  return new Intl.DateTimeFormat("en-GB", {
    timeZone: timezone,
    hour: "2-digit",
    minute: "2-digit",
    hourCycle: "h23"
  }).format(date);
}

// Analyse « HH:MM » (secondes optionnelles) en { hour, minute }, ou null.
export function parseTime(value) {
  const match = /^(\d{2}):(\d{2})(?::\d{2})?$/.exec(value);
  if (!match) return null;
  const hour = Number(match[1]);
  const minute = Number(match[2]);
  if (hour > 23 || minute > 59) return null;
  return { hour, minute };
}