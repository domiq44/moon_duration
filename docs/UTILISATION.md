# Utilisation

## Syntaxe

```bash
moon_duration [OPTIONS] [YYYY-MM-DD]
```

Sans date, le programme utilise la date locale du jour. Sans lieu, il utilise
Paris (`48.8566 N`, `2.3522 E`).

La date acceptée par le CLI va de l'an 1 à l'an 9999 (calendrier julien avant le
15 octobre 1582, grégorien ensuite). La page web restreint la saisie de 1900 à
3000.

## Options

| Option | Description |
| -------- | ------------- |
| `--lang xx` | Langue de l'interface : `fr`, `en`, `es`, `de`, `it` |
| `--lat DEG` | Latitude de l'observateur, de -90 à 90 |
| `--lon DEG` | Longitude de l'observateur, de -180 à 180 |
| `--at HH:MM` | Heure locale de l'aperçu (voir ci-dessous) |
| `--version` | Affiche la version et quitte |
| `--help` | Affiche l'aide |
| `YYYY-MM-DD` | Date locale à calculer |

Les options `--help` et `--version` sont prioritaires : elles s'affichent même
si d'autres arguments sont invalides. La langue demandée par `--lang` est
toutefois résolue pour que l'aide soit affichée dans la bonne langue.

Les coordonnées suivent une grammaire stricte : un signe « - » optionnel, des
chiffres, une partie décimale et un exposant (`4.88e1`). Les formes
hexadécimales, `inf`, `nan`, un « + » de tête ou une espace sont refusés.

Le modèle lunaire abrégé n'est garanti que de 1900 à 3000. Hors de cette plage,
le calcul est conservé mais un avertissement est écrit sur `stderr` ; la page
web, elle, restreint la date à cette plage.

## Variables d'environnement

| Variable | Effet |
| ---------- | ------- |
| `MOON_LANG` | Force la langue de l'interface |
| `LANG` | Langue du système, utilisée si `MOON_LANG` est absente |
| `TZ` | Fuseau horaire de l'affichage local |

La langue par défaut est l'anglais.

## Heures affichées

Le rapport affiche :

- la date locale et son jour de semaine ;
- les coordonnées du lieu ;
- la date et l'heure de l'aperçu (référence de la phase et du dessin) ;
- la date et l'heure locale du lever et du coucher, avec l'équivalent UTC ;
- la durée entre le lever et le coucher ;
- la phase lunaire, l'illumination et l'inclinaison du limbe à l'instant de
  l'aperçu ;
- un dessin ASCII de la phase, dont le terminateur suit l'inclinaison du limbe
  (verticale en haut, est à gauche).

L'instant de l'aperçu se déduit des arguments :

| Arguments | Instant de l'aperçu |
| --------- | ------------------- |
| ni date ni heure | date et heure d'exécution |
| `--at HH:MM` seul | jour courant à `HH:MM` |
| `YYYY-MM-DD` | cette date à midi |
| `YYYY-MM-DD --at HH:MM` | cette date et cette heure |

La durée est l'intervalle complet entre le lever et le coucher qui chevauche la
journée locale ; il peut déborder sur la veille ou le lendemain. Il s'agit de
l'intervalle complet, et non de la somme des tranches passées au-dessus de
l'horizon dans la journée. La date et l'heure de chaque événement sont affichées
en heure locale et en UTC. Lorsque la Lune reste au-dessus de l'horizon toute la
journée, la durée affichée est `24:00:00` ; lorsqu'elle reste sous l'horizon,
elle est `00:00:00`. Le mot `Indisponible` n'apparaît que si aucun des deux cas
ne peut être établi.

## Exemples

```bash
# Date du jour, Paris par défaut
moon_duration

# Une date précise
moon_duration 2026-11-14

# New York
moon_duration --lat 40.7128 --lon -74.0060 2026-11-14

# Sortie en français
MOON_LANG=fr moon_duration 2026-11-14

# Aide
moon_duration --help
```

## Page web

La démonstration web annoncée dans le [README](../README.md) permet de choisir
une date, une heure, une ville ou des coordonnées, puis affiche le lever, le
coucher, la durée, la phase, l'illumination, l'inclinaison du limbe (qui oriente
aussi le dessin ASCII) et la hauteur lunaire. La courbe distingue les zones au-dessus et
sous l'horizon, trace l'horizon lunaire en rouge pointillé et repère le maximum
par la lettre `M` ; lorsque la date affichée est celle du jour, la position
actuelle de la Lune est marquée sur la courbe. Le thème suit la préférence de
l'appareil par défaut et peut être forcé en clair ou en sombre.

Le champ `Heure` règle l'instant de l'aperçu : la phase, l'illumination et
l'inclinaison du limbe sont évaluées à cette heure locale, et la note sous les
résultats rappelle l'heure retenue. À l'ouverture, la date et l'heure sont celles
de l'instant courant ; le bouton `Maintenant` replace la date et l'heure par
celles de l'instant courant.

Pour la tester localement, compiler le module (voir
[INSTALLATION](INSTALLATION.md#page-web)) puis lancer le serveur :

```bash
make run_web
```

Le serveur local utilise le port `8000` par défaut.

La page est en français ; les cinq langues du CLI (`fr`, `en`, `es`, `de`,
`it`) ne s'appliquent qu'à la version en ligne de commande.

## Sortie

```text
Date               : 2026-11-14 (Saturday)
Location           : 48.8566 N, 2.3522 E
Preview            : 2026-11-14 12:00
Moonrise           : 2026-11-14 12:51:44 local (2026-11-14 11:51:44 UTC)
Moonset            : 2026-11-14 20:53:54 local (2026-11-14 19:53:54 UTC)
Duration           : 08:02:11 (8.04 h)
Moon phase         : Waxing Crescent
Illumination       : 21.7%
Tilt               : 302.4°

                ::::####
              :::::::#####
            :::::::::::#####
           ::::::::::::::####
          :::::::::::::::::###
           ::::::::::::::::##
            ::::::::::::::::
              ::::::::::::
                ::::::::
```

## Cas particuliers

- **Aucun lever ou coucher** : les libellés `Aucun` et `Indisponible` sont
  affichés en français (`None` et `Unavailable` en anglais). Cela se produit aux
  latitudes polaires lorsque la Lune reste sous l'horizon ou circumpolaire.
- **Coucher après minuit** : la date du coucher peut être postérieure à la date
  du rapport ; elle est indiquée explicitement.
- **Inclinaison indisponible** : le rapport affiche `Unavailable`
  (`Indisponible` en français) lorsque le limbe n'est pas identifiable, c'est-à-dire
  à la nouvelle lune et à la pleine lune. L'angle est compté depuis la verticale
  du lieu, dans le sens des angles de position ; le calcul est décrit dans
  [Algorithme lunaire](ALGORITHME.md#7-inclinaison-du-limbe). Le dessin ASCII
  retombe alors sur l'orientation de la phase : partie éclairée à droite pour une
  phase croissante, à gauche pour une phase décroissante.
- **Aperçu** : la phase, l'illumination et l'inclinaison du limbe sont évaluées
  à l'instant affiché sur la ligne `Preview` / `Aperçu`, et non au lever ou au
  coucher. Sans date ni heure, cet instant est celui de l'exécution ; avec une
  date seule, il est fixé à midi pour décrire la journée demandée.

## Version

`--version` affiche l'étiquette Git la plus proche (`v1.0.0`, ou
`v1.0.0-3-gabc1234` après trois commits). Hors dépôt Git, la valeur de repli
`0.0.0-dev` est utilisée.

```bash
git tag v1.0.0
make clean && make
./build/bin/moon_duration --version
# moon_duration v1.0.0
```

La valeur est figée dans `build/version.txt` : `main.c` n'est recompilé que
lorsque la version change.
