/*
 * test_i18n.c
 * Tests de l'internationalisation : langues, libellés, messages d'erreur et
 * chaînes d'usage.
 */

#include "cli_status.h"
#include "i18n.h"
#include "moon_phase.h"
#include "support/test_runner.h"

#include <stdlib.h>

// Vérifie que l'absence de variable d'environnement sélectionne l'anglais.
static void test_i18n_default_lang_is_english(void) {
  // Given
  unsetenv("MOON_LANG");
  unsetenv("LANG");

  // When
  i18n_init();

  // Then
  TEST_ASSERT_STRING_EQ(i18n_lang_code(), "en");
}

// Vérifie que i18n_set_lang reconnaît le français.
static void test_i18n_set_lang_french(void) {
  // Given
  // When
  int status = i18n_set_lang("fr");

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_STRING_EQ(i18n_lang_code(), "fr");
}

// Vérifie que i18n_set_lang reconnaît l'espagnol.
static void test_i18n_set_lang_spanish(void) {
  // Given
  // When
  int status = i18n_set_lang("es");

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_STRING_EQ(i18n_lang_code(), "es");
}

// Vérifie que i18n_set_lang reconnaît l'allemand.
static void test_i18n_set_lang_german(void) {
  // Given
  // When
  int status = i18n_set_lang("de");

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_STRING_EQ(i18n_lang_code(), "de");
}

// Vérifie que i18n_set_lang reconnaît l'italien.
static void test_i18n_set_lang_italian(void) {
  // Given
  // When
  int status = i18n_set_lang("it");

  // Then
  TEST_ASSERT_INT_EQ(status, 0);
  TEST_ASSERT_STRING_EQ(i18n_lang_code(), "it");
}

// Vérifie que i18n_set_lang refuse un code invalide.
static void test_i18n_set_lang_rejects_invalid_code(void) {
  // Given
  // When
  int status = i18n_set_lang("zh");

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
}

// Vérifie que i18n_set_lang refuse un pointeur NULL sans changer la langue.
static void test_i18n_set_lang_rejects_null_code(void) {
  // Given
  const char *const saved = i18n_lang_code();
  i18n_set_lang("fr");

  // When
  int status = i18n_set_lang(NULL);

  // Then
  TEST_ASSERT_INT_EQ(status, -1);
  TEST_ASSERT_STRING_EQ(i18n_lang_code(), "fr");
  i18n_set_lang(saved);
}

// Vérifie que les noms de jours sont localisés.
static void test_i18n_weekday_name_localized(void) {
  // Given
  const char *const expected_fr[] = {
      "Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};
  const char *const expected_en[] = {"Sunday",    "Monday",   "Tuesday",
                                     "Wednesday", "Thursday", "Friday",
                                     "Saturday"};
  const char *const expected_es[] = {
      "Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"};
  const char *const expected_de[] = {"Sonntag",  "Montag",     "Dienstag",
                                     "Mittwoch", "Donnerstag", "Freitag",
                                     "Samstag"};
  const char *const expected_it[] = {"Domenica",  "Lunedì",  "Martedì",
                                     "Mercoledì", "Giovedì", "Venerdì",
                                     "Sabato"};

  // When (français)
  i18n_set_lang("fr");
  // Then (français)
  for (int i = 0; i < 7; i++) {
    TEST_ASSERT_STRING_EQ(i18n_weekday_name(i), expected_fr[i]);
  }

  // When (anglais)
  i18n_set_lang("en");
  // Then (anglais)
  for (int i = 0; i < 7; i++) {
    TEST_ASSERT_STRING_EQ(i18n_weekday_name(i), expected_en[i]);
  }

  // When (espagnol)
  i18n_set_lang("es");
  // Then (espagnol)
  for (int i = 0; i < 7; i++) {
    TEST_ASSERT_STRING_EQ(i18n_weekday_name(i), expected_es[i]);
  }

  // When (allemand)
  i18n_set_lang("de");
  // Then (allemand)
  for (int i = 0; i < 7; i++) {
    TEST_ASSERT_STRING_EQ(i18n_weekday_name(i), expected_de[i]);
  }

  // When (italien)
  i18n_set_lang("it");
  // Then (italien)
  for (int i = 0; i < 7; i++) {
    TEST_ASSERT_STRING_EQ(i18n_weekday_name(i), expected_it[i]);
  }
}

// Vérifie que les noms de phases sont localisés.
static void test_i18n_moon_phase_name_localized(void) {
  // Given
  const char *const expected_fr[] = {
      "Nouvelle lune",    "Premier croissant",
      "Premier quartier", "Lune gibbeuse croissante",
      "Pleine lune",      "Lune gibbeuse décroissante",
      "Dernier quartier", "Dernier croissant"};
  const char *const expected_en[] = {
      "New Moon",  "Waxing Crescent", "First Quarter", "Waxing Gibbous",
      "Full Moon", "Waning Gibbous",  "Last Quarter",  "Waning Crescent"};
  const char *const expected_es[] = {
      "Luna nueva",       "Luna creciente",
      "Cuarto creciente", "Luna gibosa creciente",
      "Luna llena",       "Luna gibosa menguante",
      "Cuarto menguante", "Luna menguante"};
  const char *const expected_de[] = {
      "Neumond",  "Zunehmende Sichel", "Erstes Viertel",  "Zunehmender Mond",
      "Vollmond", "Abnehmender Mond",  "Letztes Viertel", "Abnehmende Sichel"};
  const char *const expected_it[] = {"Luna nuova",    "Falce di luna crescente",
                                     "Primo quarto",  "Luna gibbosa crescente",
                                     "Luna piena",    "Luna gibbosa calante",
                                     "Ultimo quarto", "Falce di luna calante"};

  // When (français)
  i18n_set_lang("fr");
  // Then (français)
  for (int i = 0; i < 8; i++) {
    TEST_ASSERT_STRING_EQ(i18n_moon_phase_name((MoonPhase)i), expected_fr[i]);
  }

  // When (anglais)
  i18n_set_lang("en");
  // Then (anglais)
  for (int i = 0; i < 8; i++) {
    TEST_ASSERT_STRING_EQ(i18n_moon_phase_name((MoonPhase)i), expected_en[i]);
  }

  // When (espagnol)
  i18n_set_lang("es");
  // Then (espagnol)
  for (int i = 0; i < 8; i++) {
    TEST_ASSERT_STRING_EQ(i18n_moon_phase_name((MoonPhase)i), expected_es[i]);
  }

  // When (allemand)
  i18n_set_lang("de");
  // Then (allemand)
  for (int i = 0; i < 8; i++) {
    TEST_ASSERT_STRING_EQ(i18n_moon_phase_name((MoonPhase)i), expected_de[i]);
  }

  // When (italien)
  i18n_set_lang("it");
  // Then (italien)
  for (int i = 0; i < 8; i++) {
    TEST_ASSERT_STRING_EQ(i18n_moon_phase_name((MoonPhase)i), expected_it[i]);
  }
}

// Vérifie que le libellé d'inclinaison est traduit dans les cinq langues.
static void test_i18n_inclination_label_localized(void) {
  // Given
  const char *const codes[I18N_COUNT] = {"fr", "en", "es", "de", "it"};
  const char *const expected[I18N_COUNT] = {
      "Inclinaison", "Tilt", "Inclinación", "Neigung", "Inclinazione"};

  // When / Then
  for (int i = 0; i < I18N_COUNT; i++) {
    TEST_ASSERT_INT_EQ(i18n_set_lang(codes[i]), 0);
    TEST_ASSERT_STRING_EQ(i18n_label_inclination(), expected[i]);
  }
}

// Vérifie que le libellé d'aperçu est traduit dans les cinq langues.
static void test_i18n_preview_label_localized(void) {
  // Given
  const char *const codes[I18N_COUNT] = {"fr", "en", "es", "de", "it"};
  const char *const expected[I18N_COUNT] = {"Aperçu", "Preview", "Vista previa",
                                            "Vorschau", "Anteprima"};

  // When / Then
  for (int i = 0; i < I18N_COUNT; i++) {
    TEST_ASSERT_INT_EQ(i18n_set_lang(codes[i]), 0);
    TEST_ASSERT_STRING_EQ(i18n_label_preview(), expected[i]);
  }
}

// Vérifie que les libellés du rapport sont localisés.
static void test_i18n_report_labels_localized(void) {
  // Given
  // When
  i18n_set_lang("fr");
  // Then (français)
  TEST_ASSERT_STRING_EQ(i18n_label_date(), "Date");
  TEST_ASSERT_STRING_EQ(i18n_label_location(), "Lieu");
  TEST_ASSERT_STRING_EQ(i18n_label_moonrise(), "Lever de la lune");
  TEST_ASSERT_STRING_EQ(i18n_label_moonset(), "Coucher de la lune");
  TEST_ASSERT_STRING_EQ(i18n_label_duration(), "Durée");
  TEST_ASSERT_STRING_EQ(i18n_label_phase(), "Phase lunaire");
  TEST_ASSERT_STRING_EQ(i18n_label_illumination(), "Illumination");
  TEST_ASSERT_STRING_EQ(i18n_label_inclination(), "Inclinaison");
  TEST_ASSERT_STRING_EQ(i18n_label_preview(), "Aperçu");
  TEST_ASSERT_STRING_EQ(i18n_label_local(), "locale");
  TEST_ASSERT_STRING_EQ(i18n_label_none(), "Aucun");
  TEST_ASSERT_STRING_EQ(i18n_label_unavailable(), "Indisponible");
  TEST_ASSERT_STRING_EQ(i18n_label_unknown(), "Inconnu");

  // When (anglais)
  i18n_set_lang("en");
  // Then (anglais)
  TEST_ASSERT_STRING_EQ(i18n_label_date(), "Date");
  TEST_ASSERT_STRING_EQ(i18n_label_location(), "Location");
  TEST_ASSERT_STRING_EQ(i18n_label_moonrise(), "Moonrise");
  TEST_ASSERT_STRING_EQ(i18n_label_moonset(), "Moonset");
  TEST_ASSERT_STRING_EQ(i18n_label_duration(), "Duration");
  TEST_ASSERT_STRING_EQ(i18n_label_phase(), "Moon phase");
  TEST_ASSERT_STRING_EQ(i18n_label_illumination(), "Illumination");
  TEST_ASSERT_STRING_EQ(i18n_label_inclination(), "Tilt");
  TEST_ASSERT_STRING_EQ(i18n_label_preview(), "Preview");
  TEST_ASSERT_STRING_EQ(i18n_label_local(), "local");
  TEST_ASSERT_STRING_EQ(i18n_label_none(), "None");
  TEST_ASSERT_STRING_EQ(i18n_label_unavailable(), "Unavailable");
  TEST_ASSERT_STRING_EQ(i18n_label_unknown(), "Unknown");
}

// Vérifie que les messages d'erreur CLI sont localisés.
static void test_i18n_error_messages_localized(void) {
  // Given
  // When
  i18n_set_lang("fr");
  // Then (français)
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_ARGUMENT_COUNT),
                        "Erreur : Trop d'arguments.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_DATE_FORMAT),
                        "Erreur : Format de date invalide. Utilisez "
                        "YYYY-MM-DD.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_DATE_RANGE),
                        "Erreur : Date invalide.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_LATITUDE),
                        "Erreur : Latitude invalide. Utilisez -90..90.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_LONGITUDE),
                        "Erreur : Longitude invalide. Utilisez -180..180.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_LANG),
                        "Erreur : Code de langue invalide. Utilisez fr, en, "
                        "es, de ou it.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_UNKNOWN_OPTION),
                        "Erreur : Option inconnue.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_CLOCK),
                        "Erreur : Impossible de lire l'horloge système.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_TIME),
                        "Erreur : Heure invalide. Utilisez HH:MM.");

  // When (anglais)
  i18n_set_lang("en");
  // Then (anglais)
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_ARGUMENT_COUNT),
                        "Error: Too many arguments.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_DATE_FORMAT),
                        "Error: Invalid date format. Use YYYY-MM-DD.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_DATE_RANGE),
                        "Error: Invalid date.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_LATITUDE),
                        "Error: Invalid latitude. Use -90..90.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_LONGITUDE),
                        "Error: Invalid longitude. Use -180..180.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_LANG),
                        "Error: Invalid language code. Use fr, en, es, de, or "
                        "it.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_UNKNOWN_OPTION),
                        "Error: Unknown option.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_CLOCK),
                        "Error: Unable to read the system clock.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_TIME),
                        "Error: Invalid time. Use HH:MM.");

  // When (allemand)
  i18n_set_lang("de");
  // Then (allemand)
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_ARGUMENT_COUNT),
                        "Fehler: Zu viele Argumente.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_LATITUDE),
                        "Fehler: Ungültige Breite. Verwendung: -90..90.");

  // When (italien)
  i18n_set_lang("it");
  // Then (italien)
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_ARGUMENT_COUNT),
                        "Errore: Troppi argomenti.");
  TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_LATITUDE),
                        "Errore: Latitudine non valida. Usa -90..90.");
}

// Vérifie qu'un index de jour invalide retourne la valeur inconnue.
static void test_i18n_weekday_name_unknown_for_invalid_index(void) {
  // Given
  i18n_set_lang("en");

  // When
  const char *name = i18n_weekday_name(-1);

  // Then
  TEST_ASSERT_STRING_EQ(name, "Unknown");
}

// Vérifie qu'une phase invalide retourne la valeur inconnue.
static void test_i18n_moon_phase_name_unknown_for_invalid_phase(void) {
  // Given
  i18n_set_lang("en");

  // When
  const char *name = i18n_moon_phase_name((MoonPhase)99);

  // Then
  TEST_ASSERT_STRING_EQ(name, "Unknown");
}

// Vérifie que le marqueur de phase indéterminée (Julian Day non fini) est
// affiché comme inconnu, et non comme une phase lunaire réelle.
static void test_i18n_moon_phase_name_unknown_for_indeterminate(void) {
  // Given
  i18n_set_lang("fr");
  const MoonPhase indeterminate = MOON_PHASE_INVALID;

  // When
  const char *name = i18n_moon_phase_name(indeterminate);

  // Then
  TEST_ASSERT_STRING_EQ(name, "Inconnu");
}

// Vérifie qu'un statut CLI invalide retourne un message vide.
// Vérifie que l'avertissement de plage de dates est localisé.
static void test_i18n_date_range_warning_localized(void) {
  // Given
  static const char *const codes[] = {"fr", "en", "es", "de", "it"};
  static const char *const messages[] = {
      "Avertissement : le modèle lunaire n'est garanti qu'entre 1900 et 3000.",
      "Warning: the lunar model is only guaranteed between 1900 and 3000.",
      "Advertencia: el modelo lunar solo está garantizado entre 1900 y 3000.",
      "Warnung: Das Mondmodell ist nur zwischen 1900 und 3000 garantiert.",
      "Avviso: il modello lunare è garantito solo tra il 1900 e il 3000.",
  };

  // When
  for (int i = 0; i < 5; i++) {
    TEST_ASSERT_INT_EQ(i18n_set_lang(codes[i]), 0);
    // Then
    TEST_ASSERT_STRING_EQ(i18n_warning_date_out_of_range(), messages[i]);
  }

  (void)i18n_set_lang("en");
}

// Vérifie que le message de durée est localisé dans les cinq langues.
static void test_i18n_duration_error_message_localized(void) {
  // Given
  static const char *const codes[] = {"fr", "en", "es", "de", "it"};
  static const char *const messages[] = {
      "Erreur : Impossible de calculer la durée.",
      "Error: Unable to compute the duration.",
      "Error: No se puede calcular la duración.",
      "Fehler: Dauer kann nicht berechnet werden.",
      "Errore: Impossibile calcolare la durata.",
  };

  // When
  for (int i = 0; i < 5; i++) {
    TEST_ASSERT_INT_EQ(i18n_set_lang(codes[i]), 0);
    // Then
    TEST_ASSERT_STRING_EQ(i18n_error_message(CLI_ERR_DURATION), messages[i]);
  }

  (void)i18n_set_lang("en");
}

static void test_i18n_error_message_rejects_invalid_status(void) {
  // Given
  i18n_set_lang("en");

  // When
  const char *message = i18n_error_message((CliStatus)999);

  // Then
  TEST_ASSERT_STRING_EQ(message, "");
}

// Vérifie que i18n_usage_strings renvoie les chaînes d'usage dans la langue
// courante, champ par champ.
static void test_i18n_usage_strings_localized(void) {
  // Given
  // When (français)
  i18n_set_lang("fr");
  const I18nUsageStrings usage_fr = i18n_usage_strings();

  // Then (français)
  TEST_ASSERT_STRING_EQ(usage_fr.header, "Usage : %s [OPTIONS] [YYYY-MM-DD]");
  TEST_ASSERT_STRING_EQ(usage_fr.description,
                        "Calcule le lever et le coucher de la Lune ainsi que "
                        "leur durée pour une date.");
  TEST_ASSERT_STRING_EQ(usage_fr.options, "Options :");
  TEST_ASSERT_STRING_EQ(usage_fr.lang,
                        "  --lang xx       Langue de l'interface (fr, en, es, "
                        "de, it)");
  TEST_ASSERT_STRING_EQ(usage_fr.lat,
                        "  --lat DEG       Latitude de l'observateur (défaut "
                        ": 48.8566)");
  TEST_ASSERT_STRING_EQ(usage_fr.lon,
                        "  --lon DEG       Longitude de l'observateur (défaut "
                        ": 2.3522)");
  TEST_ASSERT_STRING_EQ(usage_fr.version,
                        "  --version       Affiche la version");
  TEST_ASSERT_STRING_EQ(usage_fr.help, "  --help          Affiche cette aide");
  TEST_ASSERT_STRING_EQ(usage_fr.example, "Exemple : %s --lang fr 2026-11-14");

  // When (anglais)
  i18n_set_lang("en");
  const I18nUsageStrings usage_en = i18n_usage_strings();

  // Then (anglais)
  TEST_ASSERT_STRING_EQ(usage_en.header, "Usage: %s [OPTIONS] [YYYY-MM-DD]");
  TEST_ASSERT_STRING_EQ(usage_en.options, "Options:");
  TEST_ASSERT_STRING_EQ(usage_en.example, "Example: %s --lang fr 2026-11-14");
}

// Vérifie que deux appels ne partagent aucun tampon : après un changement de
// langue, la copie obtenue précédemment conserve ses chaînes d'origine.
static void test_i18n_usage_strings_copied_by_value(void) {
  // Given
  i18n_set_lang("fr");
  const I18nUsageStrings usage_fr = i18n_usage_strings();

  // When
  i18n_set_lang("en");
  const I18nUsageStrings usage_en = i18n_usage_strings();

  // Then
  TEST_ASSERT_STRING_EQ(usage_fr.header, "Usage : %s [OPTIONS] [YYYY-MM-DD]");
  TEST_ASSERT_STRING_EQ(usage_fr.example, "Exemple : %s --lang fr 2026-11-14");
  TEST_ASSERT_STRING_EQ(usage_en.header, "Usage: %s [OPTIONS] [YYYY-MM-DD]");
  TEST_ASSERT_STRING_EQ(usage_en.example, "Example: %s --lang fr 2026-11-14");
}

static const TestCase test_cases[] = {
    {"test_i18n_default_lang_is_english", test_i18n_default_lang_is_english},
    {"test_i18n_set_lang_french", test_i18n_set_lang_french},
    {"test_i18n_set_lang_spanish", test_i18n_set_lang_spanish},
    {"test_i18n_set_lang_german", test_i18n_set_lang_german},
    {"test_i18n_set_lang_italian", test_i18n_set_lang_italian},
    {"test_i18n_set_lang_rejects_invalid_code",
     test_i18n_set_lang_rejects_invalid_code},
    {"test_i18n_set_lang_rejects_null_code",
     test_i18n_set_lang_rejects_null_code},
    {"test_i18n_weekday_name_localized", test_i18n_weekday_name_localized},
    {"test_i18n_moon_phase_name_localized",
     test_i18n_moon_phase_name_localized},
    {"test_i18n_report_labels_localized", test_i18n_report_labels_localized},
    {"test_i18n_inclination_label_localized",
     test_i18n_inclination_label_localized},
    {"test_i18n_preview_label_localized", test_i18n_preview_label_localized},
    {"test_i18n_error_messages_localized", test_i18n_error_messages_localized},
    {"test_i18n_duration_error_message_localized",
     test_i18n_duration_error_message_localized},
    {"test_i18n_date_range_warning_localized",
     test_i18n_date_range_warning_localized},
    {"test_i18n_weekday_name_unknown_for_invalid_index",
     test_i18n_weekday_name_unknown_for_invalid_index},
    {"test_i18n_moon_phase_name_unknown_for_invalid_phase",
     test_i18n_moon_phase_name_unknown_for_invalid_phase},
    {"test_i18n_moon_phase_name_unknown_for_indeterminate",
     test_i18n_moon_phase_name_unknown_for_indeterminate},
    {"test_i18n_error_message_rejects_invalid_status",
     test_i18n_error_message_rejects_invalid_status},
    {"test_i18n_usage_strings_localized", test_i18n_usage_strings_localized},
    {"test_i18n_usage_strings_copied_by_value",
     test_i18n_usage_strings_copied_by_value},
};

int main(void) {
  i18n_init();
  return runner_run("i18n", test_cases, TEST_CASE_COUNT(test_cases));
}