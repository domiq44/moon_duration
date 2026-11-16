/*
 * i18n.h
 * Internationalisation : chaînes de l'interface localisées par langue.
 * Sélection via MOON_LANG, puis LANG, défaut anglais.
 */

#ifndef I18N_H
#define I18N_H

#include "cli_status.h"
#include "moon_phase.h"

typedef enum {
    I18N_FR,
    I18N_EN,
    I18N_ES,
    I18N_DE,
    I18N_IT,
    I18N_COUNT
} I18nLang;

void i18n_init(void);

int i18n_is_supported_lang(const char *code);

int i18n_set_lang(const char *code);

const char *i18n_lang_code(void);

const char *i18n_weekday_name(int weekday);

const char *i18n_moon_phase_name(MoonPhase phase);

const char *i18n_label_date(void);

const char *i18n_label_location(void);

const char *i18n_label_moonrise(void);

const char *i18n_label_moonset(void);

const char *i18n_label_duration(void);

const char *i18n_label_phase(void);

const char *i18n_label_illumination(void);

const char *i18n_label_inclination(void);

const char *i18n_label_preview(void);

const char *i18n_label_local(void);

const char *i18n_label_none(void);

const char *i18n_label_unavailable(void);

const char *i18n_label_unknown(void);

// Avertit que la date sort de la plage de validité du modèle lunaire.
const char *i18n_warning_date_out_of_range(void);

const char *i18n_error_message(CliStatus status);

typedef struct {
    const char *header;
    const char *description;
    const char *options;
    const char *lang;
    const char *lat;
    const char *lon;
    const char *at;
    const char *version;
    const char *help;
    const char *example;
} I18nUsageStrings;

// Renvoie les chaînes d'usage dans la langue courante. La structure est
// renvoyée par valeur : aucun état partagé, donc réentrance sans précaution.
I18nUsageStrings i18n_usage_strings(void);

#endif /* I18N_H */