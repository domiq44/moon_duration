/*
 * cli_status.h
 * Codes de retour de l'analyse des arguments de la ligne de commande.
 *
 * Séparé de cli.h pour que les modules de présentation (i18n) puissent
 * connaître CliStatus sans dépendre de l'analyse des arguments.
 */

#ifndef CLI_STATUS_H
#define CLI_STATUS_H

typedef enum {
    CLI_OK,
    CLI_ERR_ARGUMENT_COUNT,
    CLI_ERR_DATE_FORMAT,
    CLI_ERR_DATE_RANGE,
    CLI_ERR_LATITUDE,
    CLI_ERR_LONGITUDE,
    CLI_ERR_LANG,
    CLI_ERR_UNKNOWN_OPTION,
    CLI_ERR_CLOCK,
    CLI_ERR_TIME,
    CLI_ERR_DURATION,
    CLI_ERROR_COUNT
} CliStatus;

#endif /* CLI_STATUS_H */