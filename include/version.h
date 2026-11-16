/*
 * version.h
 * Numéro de version du programme.
 * La valeur est injectée par le Makefile depuis l'étiquette Git la plus proche
 * (release GitHub) ; ce fichier ne fournit que la valeur de repli, utilisée
 * lorsque le binaire est compilé hors d'un dépôt Git.
 */

#ifndef VERSION_H
#define VERSION_H

#ifndef MOON_VERSION
#define MOON_VERSION "0.0.0-dev"
#endif

#endif /* VERSION_H */
