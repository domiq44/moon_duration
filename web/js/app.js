// app.js — Point d'entrée de la page : chargement du noyau WebAssembly,
// formulaire, réglages, thème et rendu.

"use strict";

import { drawMoonChart } from "./chart.js?v=3";
import { createCityPicker } from "./city_picker.js?v=2";
import { findCity } from "./cities.js?v=1";
import createMoonCore from "./moon_core.mjs?v=2";
import {
  formatDateKey,
  localDateKey,
  localTimeKey,
  parseDate,
  resolveTimezone,
  todayInTimezone
} from "./date_time.js?v=12";
import { computePreview, readPreview } from "./preview.js?v=3";
import { clearPreviewErrors, renderPreview, setStatus, showPreviewError } from "./render.js?v=1";
import { clearSettings, readSettings, writeSettings } from "./settings.js?v=1";
import { applyTheme, THEME_PREFERENCES } from "./theme.js?v=6";
import { toFiniteNumber } from "./values.js?v=1";

const detectedTimezone = Intl.DateTimeFormat().resolvedOptions().timeZone || "UTC";
const colorSchemeQuery = window.matchMedia?.("(prefers-color-scheme: dark)") ?? null;

// Version de build des modules web, ajoutée aux URL pour casser le cache
// navigateur. À incrémenter avec les numéros `?v=` des imports et l'entrée
// `app.js` de `index.html` après toute modification servie.
const BUILD_VERSION = "3";
// Délai d'accalmie avant d'écrire les réglages sur disque.
const PERSIST_DELAY_MS = 200;

// Résout le binaire WebAssembly avec une version de cache dans un navigateur.
// Sous Node, les tests lisent le fichier sur le disque et la résolution par
// défaut du module est conservée.
function coreLocateFile(path) {
  if (!path.endsWith(".wasm")) return path;
  return new URL(`moon_core.wasm?v=${BUILD_VERSION}`, import.meta.url).href;
}
const elements = Object.fromEntries([
  "theme-select", "date-input", "time-input", "now-button", "city-input", "city-suggestions", "latitude-input",
  "longitude-input", "location-button", "timezone-select", "status-message",
  "result-date", "duration-value", "moonrise-value", "moonset-value",
  "illumination-value", "inclination-value", "phase-chip", "moon-ascii",
  "moon-phase-label", "preview-note",
  "moon-chart", "location-display", "settings-reset", "calculator-form"
].map((id) => [id, document.getElementById(id)]));

let moonCore = null;
let chosenCityName = "";
let themePreference = "system";
let persistTimer = null;

// Fuseau réellement utilisé : valeur de la liste ou fuseau du navigateur.
function selectedTimezone() {
  return resolveTimezone(elements["timezone-select"].value, detectedTimezone);
}

function applyCurrentTheme() {
  applyTheme(themePreference, {
    root: document.documentElement,
    themeSelect: elements["theme-select"],
    metaThemeColor: document.querySelector('meta[name="theme-color"]'),
    prefersDark: colorSchemeQuery?.matches ?? false,
    getPaperColor: () => getComputedStyle(document.documentElement).getPropertyValue("--paper").trim()
  });
}

// Place la date et l'heure sur l'instant présent, dans le fuseau choisi.
function setNow() {
  const now = new Date();
  const timezone = selectedTimezone();
  elements["date-input"].value = localDateKey(now, timezone);
  elements["time-input"].value = localTimeKey(now, timezone);
}

// Écrit immédiatement les réglages du formulaire.
function writeSettingsNow() {
  const saved = writeSettings(localStorage, {
    city: chosenCityName,
    latitude: elements["latitude-input"].value,
    longitude: elements["longitude-input"].value,
    timezone: elements["timezone-select"].value,
    theme: themePreference
  });
  if (!saved) {
    setStatus(elements, "Le navigateur ne permet pas de mémoriser les réglages.", true);
  }
}

// Mémorise les réglages après une courte accalmie : une frappe dans un champ ne
// déclenche pas une écriture disque par touche.
function persistSettings() {
  if (persistTimer !== null) {
    clearTimeout(persistTimer);
  }
  persistTimer = setTimeout(() => {
    persistTimer = null;
    writeSettingsNow();
  }, PERSIST_DELAY_MS);
}

// Force l'écriture des réglages en attente avant de quitter la page.
function flushSettings() {
  if (persistTimer === null) return;
  clearTimeout(persistTimer);
  persistTimer = null;
  writeSettingsNow();
}

// Restaure les réglages mémorisés, en ignorant les champs hors limites.
function restoreSettings() {
  const settings = readSettings(localStorage);
  if (!settings) return;

  const latitude = toFiniteNumber(settings.latitude);
  if (latitude !== null && latitude >= -90 && latitude <= 90) {
    elements["latitude-input"].value = String(latitude);
  }
  const longitude = toFiniteNumber(settings.longitude);
  if (longitude !== null && longitude >= -180 && longitude <= 180) {
    elements["longitude-input"].value = String(longitude);
  }
  if ([...elements["timezone-select"].options].some((option) => option.value === settings.timezone)) {
    elements["timezone-select"].value = settings.timezone;
  }
  const city = findCity(settings.city);
  if (city) {
    chosenCityName = city.name;
    elements["city-input"].value = city.name;
  }
  if (THEME_PREFERENCES.includes(settings.theme)) {
    themePreference = settings.theme;
  }
}

// Applique une ville choisie dans la liste.
function applyCity(city) {
  chosenCityName = city.name;
  elements["city-input"].value = city.name;
  elements["latitude-input"].value = city.latitude.toFixed(4);
  elements["longitude-input"].value = city.longitude.toFixed(4);
  elements["timezone-select"].value = city.timezone;
  update();
}

// Bascule sur des coordonnées saisies à la main.
function switchToCustomLocation() {
  chosenCityName = "";
  elements["city-input"].value = "";
  cityPicker.close();
}

function resetSettings() {
  if (!clearSettings(localStorage)) {
    setStatus(elements, "Le navigateur ne permet pas de supprimer les réglages mémorisés.", true);
  }
  switchToCustomLocation();
  elements["latitude-input"].value = "48.8566";
  elements["longitude-input"].value = "2.3522";
  elements["timezone-select"].value = "auto";
  themePreference = "system";
  applyCurrentTheme();
  setNow();
  update();
}

// Valide la saisie, calcule l'aperçu puis met à jour la page.
function update() {
  if (!moonCore) return;
  clearPreviewErrors(elements);
  try {
    renderCurrentPreview();
  } catch (error) {
    // Une erreur inattendue du noyau ou du rendu ne doit pas interrompre la
    // page : elle est signalée à la place de l'aperçu.
    console.error("Échec du calcul de l’aperçu", error);
    setStatus(elements, "Le calcul de l’aperçu a échoué.", true);
  }
}

// Calcule et affiche l'aperçu pour la saisie courante.
function renderCurrentPreview() {
  const preview = readPreview({
    dateValue: elements["date-input"].value,
    timeValue: elements["time-input"].value,
    latitudeText: elements["latitude-input"].value,
    longitudeText: elements["longitude-input"].value
  });
  if (!preview.ok) {
    showPreviewError(elements, preview.error);
    return;
  }

  const timezone = selectedTimezone();
  const result = computePreview(moonCore, preview, timezone);
  renderPreview(elements, {
    result,
    date: preview.date,
    time: preview.time,
    timezone,
    locationName: chosenCityName
  });
  setStatus(elements, "");
  persistSettings();
  drawMoonChart(elements["moon-chart"], {
    altitudeAt: (hour) => moonCore._moon_web_altitude(
      result.dayStartJulianDay + hour / 24, preview.latitude, preview.longitude),
    dayStartJulianDay: result.dayStartJulianDay,
    currentJulianDay: result.currentJulianDay,
    timezone,
    selectedDateKey: result.dateKey,
    latitude: preview.latitude,
    longitude: preview.longitude,
    dayHours: result.dayHours
  });
}

const cityPicker = createCityPicker({
  input: elements["city-input"],
  list: elements["city-suggestions"],
  onSelect: applyCity
});

const autoTimezoneOption =
  elements["timezone-select"].querySelector('option[value="auto"]');
if (autoTimezoneOption) {
  autoTimezoneOption.textContent = `Automatique (${detectedTimezone})`;
}

restoreSettings();
setNow();
applyCurrentTheme();

elements["theme-select"].addEventListener("change", () => {
  themePreference = elements["theme-select"].value;
  applyCurrentTheme();
  persistSettings();
  update();
});
colorSchemeQuery?.addEventListener("change", () => {
  if (themePreference === "system") {
    applyCurrentTheme();
    update();
  }
});
elements["now-button"].addEventListener("click", () => {
  setNow();
  update();
});
elements["date-input"].addEventListener("input", update);
elements["time-input"].addEventListener("input", update);
elements["calculator-form"].addEventListener("submit", (event) => {
  event.preventDefault();
  update();
});
elements["settings-reset"].addEventListener("click", resetSettings);
window.addEventListener("pagehide", flushSettings);
elements["timezone-select"].addEventListener("change", update);
elements["city-input"].addEventListener("focus", cityPicker.render);
elements["city-input"].addEventListener("blur", cityPicker.close);
elements["city-input"].addEventListener("input", () => {
  chosenCityName = "";
  cityPicker.refresh();
  update();
});
elements["city-input"].addEventListener("keydown", cityPicker.handleKeydown);
// Un clic hors du sélecteur referme la liste de suggestions.
document.addEventListener("pointerdown", (event) => {
  if (!event.target.closest(".city-picker")) {
    cityPicker.close();
  }
});
elements["city-suggestions"].addEventListener("pointerdown", (event) => event.preventDefault());
elements["city-suggestions"].addEventListener("click", cityPicker.handleClick);
elements["latitude-input"].addEventListener("input", () => {
  switchToCustomLocation();
  update();
});
elements["longitude-input"].addEventListener("input", () => {
  switchToCustomLocation();
  update();
});
elements["location-button"].addEventListener("click", () => {
  if (!navigator.geolocation) {
    setStatus(elements, "La géolocalisation n’est pas disponible dans ce navigateur.", true);
    return;
  }
  setStatus(elements, "Recherche de la position…");
  navigator.geolocation.getCurrentPosition(
    (position) => {
      switchToCustomLocation();
      elements["latitude-input"].value = position.coords.latitude.toFixed(4);
      elements["longitude-input"].value = position.coords.longitude.toFixed(4);
      update();
    },
    () => setStatus(elements, "Position indisponible. Saisissez la latitude ou la longitude.", true),
    { enableHighAccuracy: false, timeout: 8000, maximumAge: 300000 }
  );
});
new ResizeObserver(update).observe(elements["moon-chart"]);
setInterval(() => {
  const date = parseDate(elements["date-input"].value);
  if (!date) return;
  if (todayInTimezone(selectedTimezone()) === formatDateKey(date)) {
    update();
  }
}, 60000);

createMoonCore(
  import.meta.url.startsWith("file:") ? {} : { locateFile: coreLocateFile }
).then((module) => {
  moonCore = module;
  update();
}).catch(() => {
  setStatus(elements, "Le module de calcul WebAssembly n’a pas pu être chargé.", true);
});