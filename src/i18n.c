/*
 * i18n.c
 * Internationalisation : chaînes de l'interface localisées par langue.
 * Les traductions sont regroupées dans des tableaux statiques.
 * Sélection via MOON_LANG, puis LANG, défaut anglais.
 */

#include "i18n.h"

#include <stdlib.h>
#include <string.h>

#include "cli_status.h"
#include "moon_phase.h"

typedef struct {
  const char *weekday[7];
  const char *phase[8];
  const char *label_date;
  const char *label_location;
  const char *label_moonrise;
  const char *label_moonset;
  const char *label_duration;
  const char *label_phase;
  const char *label_illumination;
  const char *label_inclination;
  const char *label_preview;
  const char *label_local;
  const char *label_none;
  const char *label_unavailable;
  const char *label_unknown;
  const char *warning_date_range;
  const char *error[CLI_ERROR_COUNT - 1];
  const char *usage_header;
  const char *usage_description;
  const char *usage_options;
  const char *usage_lang;
  const char *usage_lat;
  const char *usage_lon;
  const char *usage_at;
  const char *usage_version;
  const char *usage_help;
  const char *usage_example;
} LangStrings;

static const char *const lang_codes[I18N_COUNT] = {
    "fr", "en", "es", "de", "it",
};

static const LangStrings lang_tables[I18N_COUNT] = {
    {
        {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi",
         "Samedi"},
        {"Nouvelle lune", "Premier croissant", "Premier quartier",
         "Lune gibbeuse croissante", "Pleine lune",
         "Lune gibbeuse décroissante", "Dernier quartier", "Dernier croissant"},
        "Date",
        "Lieu",
        "Lever de la lune",
        "Coucher de la lune",
        "Durée",
        "Phase lunaire",
        "Illumination",
        "Inclinaison",
        "Aperçu",
        "locale",
        "Aucun",
        "Indisponible",
        "Inconnu",
        "Avertissement : le modèle lunaire n'est garanti qu'entre 1900 et "
        "3000.",
        {"Erreur : Trop d'arguments.",
         "Erreur : Format de date invalide. Utilisez YYYY-MM-DD.",
         "Erreur : Date invalide.",
         "Erreur : Latitude invalide. Utilisez -90..90.",
         "Erreur : Longitude invalide. Utilisez -180..180.",
         "Erreur : Code de langue invalide. Utilisez fr, en, es, de ou it.",
         "Erreur : Option inconnue.",
         "Erreur : Impossible de lire l'horloge système.",
         "Erreur : Heure invalide. Utilisez HH:MM.",
         "Erreur : Impossible de calculer la durée."},
        "Usage : %s [OPTIONS] [YYYY-MM-DD]",
        "Calcule le lever et le coucher de la Lune ainsi que leur durée pour "
        "une date.",
        "Options :",
        "  --lang xx       Langue de l'interface (fr, en, es, de, it)",
        "  --lat DEG       Latitude de l'observateur (défaut : 48.8566)",
        "  --lon DEG       Longitude de l'observateur (défaut : 2.3522)",
        "  --at HH:MM      Heure locale de l'aperçu (défaut : midi ou "
        "maintenant)",
        "  --version       Affiche la version",
        "  --help          Affiche cette aide",
        "Exemple : %s --lang fr 2026-11-14",
    },
    {
        {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
         "Saturday"},
        {"New Moon", "Waxing Crescent", "First Quarter", "Waxing Gibbous",
         "Full Moon", "Waning Gibbous", "Last Quarter", "Waning Crescent"},
        "Date",
        "Location",
        "Moonrise",
        "Moonset",
        "Duration",
        "Moon phase",
        "Illumination",
        "Tilt",
        "Preview",
        "local",
        "None",
        "Unavailable",
        "Unknown",
        "Warning: the lunar model is only guaranteed between 1900 and 3000.",
        {"Error: Too many arguments.",
         "Error: Invalid date format. Use YYYY-MM-DD.", "Error: Invalid date.",
         "Error: Invalid latitude. Use -90..90.",
         "Error: Invalid longitude. Use -180..180.",
         "Error: Invalid language code. Use fr, en, es, de, or it.",
         "Error: Unknown option.", "Error: Unable to read the system clock.",
         "Error: Invalid time. Use HH:MM.",
         "Error: Unable to compute the duration."},
        "Usage: %s [OPTIONS] [YYYY-MM-DD]",
        "Compute moonrise, moonset, and their duration for a date.",
        "Options:",
        "  --lang xx       Interface language (fr, en, es, de, it)",
        "  --lat DEG       Observer latitude (default: 48.8566)",
        "  --lon DEG       Observer longitude (default: 2.3522)",
        "  --at HH:MM      Local time of the preview (default: noon or now)",
        "  --version       Show the version",
        "  --help          Show this help",
        "Example: %s --lang fr 2026-11-14",
    },
    {
        {"Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes",
         "Sábado"},
        {"Luna nueva", "Luna creciente", "Cuarto creciente",
         "Luna gibosa creciente", "Luna llena", "Luna gibosa menguante",
         "Cuarto menguante", "Luna menguante"},
        "Fecha",
        "Ubicación",
        "Salida de la luna",
        "Puesta de la luna",
        "Duración",
        "Fase lunar",
        "Iluminación",
        "Inclinación",
        "Vista previa",
        "local",
        "Ninguna",
        "No disponible",
        "Desconocido",
        "Advertencia: el modelo lunar solo está garantizado entre 1900 y 3000.",
        {"Error: Demasiados argumentos.",
         "Error: Formato de fecha no válido. Use YYYY-MM-DD.",
         "Error: Fecha no válida.", "Error: Latitud no válida. Use -90..90.",
         "Error: Longitud no válida. Use -180..180.",
         "Error: Código de idioma no válido. Use fr, en, es, de o it.",
         "Error: Opción desconocida.",
         "Error: No se puede leer el reloj del sistema.",
         "Error: Hora no válida. Use HH:MM.",
         "Error: No se puede calcular la duración."},
        "Uso: %s [OPCIONES] [YYYY-MM-DD]",
        "Calcula la salida y la puesta de la Luna y su duración para una "
        "fecha.",
        "Opciones:",
        "  --lang xx       Idioma de la interfaz (fr, en, es, de, it)",
        "  --lat DEG       Latitud del observador (por defecto: 48.8566)",
        "  --lon DEG       Longitud del observador (por defecto: 2.3522)",
        "  --at HH:MM      Hora local de la vista previa (por defecto: "
        "mediodía o ahora)",
        "  --version       Muestra la versión",
        "  --help          Muestra esta ayuda",
        "Ejemplo: %s --lang fr 2026-11-14",
    },
    {
        {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag",
         "Samstag"},
        {"Neumond", "Zunehmende Sichel", "Erstes Viertel", "Zunehmender Mond",
         "Vollmond", "Abnehmender Mond", "Letztes Viertel",
         "Abnehmende Sichel"},
        "Datum",
        "Standort",
        "Mondaufgang",
        "Monduntergang",
        "Dauer",
        "Mondphase",
        "Beleuchtung",
        "Neigung",
        "Vorschau",
        "lokal",
        "Keine",
        "Nicht verfügbar",
        "Unbekannt",
        "Warnung: Das Mondmodell ist nur zwischen 1900 und 3000 garantiert.",
        {"Fehler: Zu viele Argumente.",
         "Fehler: Ungültiges Datumsformat. Verwendung: YYYY-MM-DD.",
         "Fehler: Ungültiges Datum.",
         "Fehler: Ungültige Breite. Verwendung: -90..90.",
         "Fehler: Ungültige Länge. Verwendung: -180..180.",
         "Fehler: Ungültiger Sprachcode. Verwendung: fr, en, es, de oder it.",
         "Fehler: Unbekannte Option.",
         "Fehler: Systemuhr kann nicht gelesen werden.",
         "Fehler: Ungültige Uhrzeit. Verwendung: HH:MM.",
         "Fehler: Dauer kann nicht berechnet werden."},
        "Verwendung: %s [OPTIONEN] [YYYY-MM-DD]",
        "Berechnet Mondaufgang, Monduntergang und deren Dauer für ein Datum.",
        "Optionen:",
        "  --lang xx       Oberflächensprache (fr, en, es, de, it)",
        "  --lat DEG       Breite des Beobachters (Standard: 48.8566)",
        "  --lon DEG       Länge des Beobachters (Standard: 2.3522)",
        "  --at HH:MM      Ortszeit der Vorschau (Standard: Mittag oder jetzt)",
        "  --version       Zeigt die Version",
        "  --help          Zeigt diese Hilfe",
        "Beispiel: %s --lang fr 2026-11-14",
    },
    {
        {"Domenica", "Lunedì", "Martedì", "Mercoledì", "Giovedì", "Venerdì",
         "Sabato"},
        {"Luna nuova", "Falce di luna crescente", "Primo quarto",
         "Luna gibbosa crescente", "Luna piena", "Luna gibbosa calante",
         "Ultimo quarto", "Falce di luna calante"},
        "Data",
        "Posizione",
        "Levata della luna",
        "Tramonto della luna",
        "Durata",
        "Fase lunare",
        "Illuminazione",
        "Inclinazione",
        "Anteprima",
        "locale",
        "Nessuna",
        "Non disponibile",
        "Sconosciuto",
        "Avviso: il modello lunare è garantito solo tra il 1900 e il 3000.",
        {"Errore: Troppi argomenti.",
         "Errore: Formato data non valido. Usa YYYY-MM-DD.",
         "Errore: Data non valida.",
         "Errore: Latitudine non valida. Usa -90..90.",
         "Errore: Longitudine non valida. Usa -180..180.",
         "Errore: Codice lingua non valido. Usa fr, en, es, de o it.",
         "Errore: Opzione sconosciuta.",
         "Errore: Impossibile leggere l'orologio di sistema.",
         "Errore: Ora non valida. Usa HH:MM.",
         "Errore: Impossibile calcolare la durata."},
        "Uso: %s [OPZIONI] [YYYY-MM-DD]",
        "Calcola levata, tramonto della Luna e la loro durata per una data.",
        "Opzioni:",
        "  --lang xx       Lingua dell'interfaccia (fr, en, es, de, it)",
        "  --lat DEG       Latitudine dell'osservatore (predefinito: 48.8566)",
        "  --lon DEG       Longitudine dell'osservatore (predefinito: 2.3522)",
        "  --at HH:MM      Ora locale dell'anteprima (predefinito: mezzogiorno "
        "o adesso)",
        "  --version       Mostra la versione",
        "  --help          Mostra questo aiuto",
        "Esempio: %s --lang fr 2026-11-14",
    },
};

static I18nLang current_lang = I18N_EN;

static I18nLang parse_lang_code(const char *code) {
  if (code == NULL) {
    return I18N_COUNT;
  }

  for (int i = 0; i < I18N_COUNT; i++) {
    if (strcmp(code, lang_codes[i]) == 0) {
      return (I18nLang)i;
    }
  }
  return I18N_COUNT;
}

static I18nLang detect_lang_from_env(const char *env_value) {
  char buffer[16];
  size_t i = 0;

  if (env_value == NULL || *env_value == '\0') {
    return I18N_COUNT;
  }

  while (env_value[i] != '\0' && env_value[i] != '_' && env_value[i] != '.' &&
         env_value[i] != '@' && i < sizeof(buffer) - 1) {
    buffer[i] = env_value[i];
    i++;
  }
  buffer[i] = '\0';

  return parse_lang_code(buffer);
}

static const LangStrings *current_strings(void) {
  return &lang_tables[current_lang];
}

void i18n_init(void) {
  I18nLang lang = detect_lang_from_env(getenv("MOON_LANG"));

  if (lang == I18N_COUNT) {
    lang = detect_lang_from_env(getenv("LANG"));
  }

  if (lang == I18N_COUNT) {
    lang = I18N_EN;
  }

  current_lang = lang;
}

int i18n_is_supported_lang(const char *code) {
  return code != NULL && parse_lang_code(code) != I18N_COUNT;
}

int i18n_set_lang(const char *code) {
  I18nLang lang = parse_lang_code(code);

  if (lang == I18N_COUNT) {
    return -1;
  }

  current_lang = lang;
  return 0;
}

const char *i18n_lang_code(void) { return lang_codes[current_lang]; }

const char *i18n_weekday_name(int weekday) {
  if (weekday < 0 || weekday > 6) {
    return current_strings()->label_unknown;
  }
  return current_strings()->weekday[weekday];
}

const char *i18n_moon_phase_name(MoonPhase phase) {
  if (phase < MOON_NEW || phase > MOON_WANING_CRESCENT) {
    return current_strings()->label_unknown;
  }
  return current_strings()->phase[(int)phase];
}

const char *i18n_label_date(void) { return current_strings()->label_date; }

const char *i18n_label_location(void) {
  return current_strings()->label_location;
}

const char *i18n_label_moonrise(void) {
  return current_strings()->label_moonrise;
}

const char *i18n_label_moonset(void) {
  return current_strings()->label_moonset;
}

const char *i18n_label_duration(void) {
  return current_strings()->label_duration;
}

const char *i18n_label_phase(void) { return current_strings()->label_phase; }

const char *i18n_label_illumination(void) {
  return current_strings()->label_illumination;
}

const char *i18n_label_inclination(void) {
  return current_strings()->label_inclination;
}

const char *i18n_label_preview(void) {
  return current_strings()->label_preview;
}

const char *i18n_label_local(void) { return current_strings()->label_local; }

const char *i18n_label_none(void) { return current_strings()->label_none; }

const char *i18n_label_unavailable(void) {
  return current_strings()->label_unavailable;
}

const char *i18n_label_unknown(void) {
  return current_strings()->label_unknown;
}

const char *i18n_warning_date_out_of_range(void) {
  return current_strings()->warning_date_range;
}

const char *i18n_error_message(CliStatus status) {
  const int index = (int)status - 1;

  if (status <= CLI_OK || index >= CLI_ERROR_COUNT - 1) {
    return "";
  }

  return current_strings()->error[index];
}

I18nUsageStrings i18n_usage_strings(void) {
  const LangStrings *strings = current_strings();

  I18nUsageStrings usage = {
      .header = strings->usage_header,
      .description = strings->usage_description,
      .options = strings->usage_options,
      .lang = strings->usage_lang,
      .lat = strings->usage_lat,
      .lon = strings->usage_lon,
      .at = strings->usage_at,
      .version = strings->usage_version,
      .help = strings->usage_help,
      .example = strings->usage_example,
  };

  return usage;
}