/*
 * cli.h
 * Analyse, validation et messages d'usage des arguments de la ligne de commande.
 */

#ifndef CLI_H
#define CLI_H

#include <stdio.h>

#include "cli_status.h"
#include "datetime.h"

typedef struct {
    DateTime date;
    int has_date;
    int has_time;
    int hour;
    int minute;
    int show_help;
    int show_version;
    double latitude;
    double longitude;
    char lang[8];
} CliArgs;

CliStatus cli_parse(int argc, char **argv, CliArgs *args);

void cli_print_error(FILE *stream, CliStatus status);

void cli_print_usage(FILE *stream, const char *program);

// Exécute la commande complète : analyse, calcul du rapport et sortie.
// Séparée de main pour permettre les tests de bout en bout.
int run_cli(int argc, char *argv[], FILE *out, FILE *err);

#endif /* CLI_H */
