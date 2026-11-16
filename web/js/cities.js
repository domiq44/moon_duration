// cities.js — Catalogue des villes et recherche par nom, insensible aux accents
// et à la casse.

"use strict";

import { normalizeText } from "./values.js?v=1";

// Villes proposées par l'outil, avec leur fuseau horaire.
export const CITIES = Object.freeze([
  { name: "Ajaccio", latitude: 41.9192, longitude: 8.7386, timezone: "Europe/Paris" },
  { name: "Amsterdam", latitude: 52.3676, longitude: 4.9041, timezone: "Europe/Amsterdam" },
  { name: "Berlin", latitude: 52.52, longitude: 13.405, timezone: "Europe/Berlin" },
  { name: "Bordeaux", latitude: 44.8378, longitude: -0.5792, timezone: "Europe/Paris" },
  { name: "Brest", latitude: 48.3904, longitude: -4.4861, timezone: "Europe/Paris" },
  { name: "Bruxelles", latitude: 50.8503, longitude: 4.3517, timezone: "Europe/Paris" },
  { name: "Genève", latitude: 46.2044, longitude: 6.1432, timezone: "Europe/Paris" },
  { name: "Lille", latitude: 50.6292, longitude: 3.0573, timezone: "Europe/Paris" },
  { name: "Lisbonne", latitude: 38.7223, longitude: -9.1393, timezone: "Europe/Lisbon" },
  { name: "Londres", latitude: 51.5072, longitude: -0.1276, timezone: "Europe/London" },
  { name: "Lyon", latitude: 45.764, longitude: 4.8357, timezone: "Europe/Paris" },
  { name: "Madrid", latitude: 40.4168, longitude: -3.7038, timezone: "Europe/Madrid" },
  { name: "Marseille", latitude: 43.2965, longitude: 5.3698, timezone: "Europe/Paris" },
  { name: "Milan", latitude: 45.4642, longitude: 9.19, timezone: "Europe/Rome" },
  { name: "Montpellier", latitude: 43.6108, longitude: 3.8767, timezone: "Europe/Paris" },
  { name: "Nantes", latitude: 47.2184, longitude: -1.5536, timezone: "Europe/Paris" },
  { name: "New York", latitude: 40.7128, longitude: -74.006, timezone: "America/New_York" },
  { name: "Nice", latitude: 43.7102, longitude: 7.262, timezone: "Europe/Paris" },
  { name: "Paris", latitude: 48.8566, longitude: 2.3522, timezone: "Europe/Paris" },
  { name: "Prague", latitude: 50.0755, longitude: 14.4378, timezone: "Europe/Prague" },
  { name: "Rome", latitude: 41.9028, longitude: 12.4964, timezone: "Europe/Rome" },
  { name: "Strasbourg", latitude: 48.5734, longitude: 7.7521, timezone: "Europe/Paris" },
  { name: "Stockholm", latitude: 59.3293, longitude: 18.0686, timezone: "Europe/Stockholm" },
  { name: "Tokyo", latitude: 35.6762, longitude: 139.6503, timezone: "Asia/Tokyo" },
  { name: "Toulouse", latitude: 43.6047, longitude: 1.4442, timezone: "Europe/Paris" },
  { name: "Los Angeles", latitude: 34.0522, longitude: -118.2437, timezone: "America/Los_Angeles" },
  { name: "Sydney", latitude: -33.8688, longitude: 151.2093, timezone: "Australia/Sydney" }
]);

// Ville dont le nom correspond exactement, ou null.
export function findCity(name) {
  return CITIES.find((city) => city.name === name) ?? null;
}

// Villes dont le nom contient la recherche, sans accents, au plus `limit`.
export function searchCities(query, limit = 8) {
  const normalized = normalizeText(query.trim());
  return CITIES.filter((city) => normalizeText(city.name).includes(normalized)).slice(0, limit);
}