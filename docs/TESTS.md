# Tests unitaires et intégration

## Commandes

| Cible | Effet |
| ------- | ------- |
| `make test` | suites C, précédées de `check-test-sources` |
| `make test_web_wasm` | module WebAssembly (Node.js) |
| `make test_date_time` | dates, heures et fuseaux web (Node.js) |
| `make test_values` | conversions et normalisation de valeurs (Node.js) |
| `make test_format` | formatage de la sortie web (Node.js) |
| `make test_cities` | catalogue et recherche des villes (Node.js) |
| `make test_city_picker` | navigation clavier et ARIA de l'autocomplétion (Node.js) |
| `make test_settings` | stockage des réglages (Node.js) |
| `make test_preview` | validation et calcul de l'aperçu (Node.js) |
| `make test_preview_offsets` | décalages et durée des jours 23 h/25 h (Node.js) |
| `make test_app` | assemblage de la page avec un DOM factice (Node.js) |
| `make test_chart` | graphique web (Node.js) |
| `make test_theme` | thème clair/sombre (Node.js) |
| `make test_sanitize` | suites C + fuzzing sous ASan/UBSan |
| `make fuzz` | harnais de fuzzing seul |
| `make coverage` | rapport gcov |
| `make lint` | clang-tidy |
| `make cppcheck` | cppcheck |
| `make format` / `make format-check` | clang-format (réécrit / vérifie) |

```bash
make test
```

## Structure des tests (AAA)

Chaque test suit la structure **AAA**, avec des sections `Given`, `When` et
`Then` :

- **Given** : déclare les entrées, les résultats attendus et le contexte.
- **When** : effectue l'action principale.
- **Then** : vérifie les résultats observables.

Les commentaires des tests sont rédigés en français. Les balises AAA peuvent
rester en anglais.

## Organisation

- Préfixe `test_` réservé aux fonctions de test.
- Helpers placés avant les cas de test dans le même fichier.
- Pour les suites C, le runner déclare les cas dans un tableau et délègue
  l'exécution, le reporting et le bilan au runner partagé
  (`tests/support/test_runner.c`).
- Le runner utilise les statuts `PASS`, `FAIL` et `SKIP`, calcule les totaux et
  enchaîne les cas.
- Une assertion qui échoue interrompt le cas courant (`return` dans la macro) :
  la suite ne poursuit pas sur un état déjà invalide, ce qui évite qu'un crash
  masque le `FAIL` d'origine.
- Tout fichier `tests/test_*.c` doit figurer dans `TEST_SRCS` (Makefile).
  `make check-test-sources` échoue sinon ; la cible est une dépendance de
  `make test`, si bien qu'un fichier de test orphelin ne peut plus passer
  inaperçu.

## Suites de tests

| Suite | Contenu |
| ------- | --------- |
| `test_datetime` | Conversions Julian Day (julien et grégorien, aller et retour), jours de semaine, validation, ajout de jours et bornes entières |
| `test_clock` | Heure courante, source d'horloge injectable, conversions local/UTC avec décalages signés et aller-retour pré-grégorien |
| `test_astro` | Normalisation des angles, trigonométrie en degrés, arc sinus borné, conversion écliptique → équatoriale et temps sidéral |
| `test_moon_position` | Position lunaire, distance, altitude, référence JPL |
| `test_sun_position` | Position solaire, plages physiques, référence JPL et équation du centre |
| `test_moon_events` | Détection des levers/couchers à pas resserré près de l'horizon (marge 1,3°), type de traversée, cas polaires, effleurement de l'horizon et tampon tronqué |
| `test_moon_duration` | Calcul de durée, franchissement de minuit, Lune circumpolaire (24 h), Lune sous l'horizon (0 h) et journée à deux tranches |
| `test_moon_duration_reference` | Comparaison avec les tables RTS de JPL Horizons (plusieurs villes, années 1900–3000) et précision globale |
| `test_moon_phase` | Huit phases dans l'ordre, illumination par élongation, rejet des Julian Day non finis et repli synodique |
| `test_moon_orientation` | Angle du limbe brillant (géocentrique et apparent), angle parallactique et cas indéterminés |
| `test_moon_ascii` | Rendu ASCII du disque lunaire, orientation du terminateur et silhouette |
| `test_cli` | Analyse et validation stricte des arguments, dont `--at HH:MM`, les minutes qui débordent `int`, les signes/espaces refusés, l'avertissement de plage de dates et la priorité de `--help`/`--version` |
| `test_i18n` | Internationalisation, libellés inconnus, phase indéterminée, avertissement de plage et chaînes d'usage renvoyées par valeur |
| `test_report` | Formatage de la sortie, instant d'aperçu, durées de 24 h et 0 h, dessin ASCII sans espaces de fin |
| `test_web_api` | API C du calculateur WebAssembly : fenêtre locale à deux offsets, aperçu daté, rendu ASCII à tampon fourni, mode de durée (jour entier ou Lune absente) et valeurs indéterminées (NaN) |
| `test_web_pages` | Contrat de la page de démonstration : CSP, module, absence de script en ligne et instant d'aperçu |
| `test_integration` | Exécution du binaire CLI (bout en bout), dont l'application de `--lang`, l'aperçu daté et les durées polaires de 24 h et 0 h |

La suite C compte 290 cas de test.

Les suites C vivent à la racine de `tests/`, les suites Node du frontal web dans
`tests/web/` : les deux chaînes d'outils (runner C et `node --test`) sont ainsi
séparées, et le dossier `web/` publié par GitHub Pages ne contient aucun test.

Le module WebAssembly est testé séparément avec Node.js (18 tests) :

```bash
make test_web_wasm
```

`tests/web/test_web_wasm.mjs` utilise la structure `Given`/`When`/`Then` et vérifie
le jour julien, la durée, les événements, la phase, l'illumination, l'inclinaison
du limbe, l'aperçu à une heure donnée (`moon_web_*_at`), les entrées invalides, le
rendu ASCII et son orientation, ainsi que la fenêtre locale (lever, coucher et
durée) avec ses deux offsets de début et de fin.

Les aides de date et de fuseau horaire sont testées séparément avec Node.js
(22 tests) :

```bash
make test_date_time
```

`tests/web/test_date_time.mjs` vérifie le parsing des dates et des heures
(`parseDate`, `parseTime`), les offsets horaires à midi et à l'instant
(`timezoneOffsetMinutes`, `instantOffsetMinutes`), la conversion Julian Day, les
clés de date locale et `localDayOffsets`, y compris les jours de 23 h et 25 h,
ainsi que la résolution du fuseau (`resolveTimezone`).

Les conversions de valeurs sont testées séparément avec Node.js (3 tests) :

```bash
make test_values
```

`tests/web/test_values.mjs` vérifie `toFiniteNumber` (rejet de `null`, `""`, `" "` et
des booléens) et la normalisation du texte (`normalizeText`).

Le formatage web est testé séparément avec Node.js (13 tests) :

```bash
make test_format
```

`tests/web/test_format.mjs` vérifie le formatage des événements (heure seule et
décalage de jour), de la durée, des libellés de l'axe (`hourTickLabels`, y compris
les jours de 23 h et 25 h), de l'illumination (pourcentage à une décimale) et de
l'inclinaison (degrés à une décimale, ramenés dans [0, 360)).

Le catalogue des villes est testé séparément avec Node.js (4 tests) :

```bash
make test_cities
```

`tests/web/test_cities.mjs` vérifie la recherche exacte (`findCity`) et la recherche
partielle insensible aux accents et à la casse (`searchCities`), avec sa limite de
résultats.

Les réglages mémorisés sont testés séparément avec Node.js (4 tests) :

```bash
make test_settings
```

`tests/web/test_settings.mjs` vérifie la lecture, l'écriture et la suppression des
réglages avec un stockage factice, ainsi que le rejet d'un stockage absent,
corrompu ou refusé par le navigateur.

L'aperçu web est testé séparément avec Node.js (10 tests) :

```bash
make test_preview
```

`tests/web/test_preview.mjs` vérifie la validation des champs (`readPreview`), le
calcul de l'aperçu parisien (`computePreview`) et le mode de durée : durée, phase,
illumination, inclinaison, fenêtre locale, dessin ASCII, instant « Maintenant »,
jour entier (24 h) et Lune absente (0 h).

Les décalages de début et de fin de journée sont testés sans WebAssembly avec
Node.js (6 tests) :

```bash
make test_preview_offsets
```

`tests/web/test_preview_offsets.mjs` utilise un noyau factice et vérifie que les
deux décalages sont transmis au calcul, que le début de journée utilise le
décalage de début, que la phase, l'illumination et l'inclinaison utilisent
l'offset de l'instant demandé (et non l'offset de midi), et que la durée écoulée
vaut 23 h, 24 h ou 25 h selon le changement d'heure.

La page assemblée est testée avec Node.js (4 tests) :

```bash
make test_app
```

`tests/web/test_app.mjs` importe `web/js/app.js` avec un DOM factice et vérifie le calcul
de bout en bout : la saisie parisienne du 14 novembre 2026 à 20:30 produit la
durée, la phase, l'illumination, l'inclinaison et la note attendues. Il vérifie
aussi l'écriture différée des réglages, leur vidage sur `pagehide` et le message
affiché lorsqu'une erreur inattendue survient pendant le calcul.

L'autocomplétion est testée séparément avec Node.js (10 tests) :

```bash
make test_city_picker
```

`tests/web/test_city_picker.mjs` vérifie la navigation clavier (`ArrowDown`,
`ArrowUp`, bouclage), la sélection par `Entrée` ou par clic, la fermeture par
`Échap` ou `close`, la réouverture après rendu, l'effacement de
`aria-activedescendant` lors d'une nouvelle saisie et le masquage de la liste
sans résultat.

Le graphique web est testé séparément avec Node.js (15 tests) :

```bash
make test_chart
```

`tests/web/test_chart.mjs` vérifie la recherche du maximum de la courbe, les
intervalles au-dessus de l'horizon (`findIntervals`), le repère vertical avec la
lettre `M` et la ligne d'horizon rouge pointillée.

Le thème clair/sombre est testé séparément avec Node.js (5 tests) :

```bash
make test_theme
```

`tests/web/test_theme.mjs` vérifie la résolution du thème système et l'application
des thèmes clair et sombre.

## Sanitaires et fuzzing

```bash
make test_sanitize          # suites C + harnais sous ASan/UBSan
make fuzz                   # harnais seul
FUZZ_ITERS=100000 make fuzz # nombre de tours
FUZZ_SEED=42 make fuzz      # graine reproductible
```

Les suites sont recompilées dans `build/asan/` avec
`-fsanitize=address,undefined -fno-sanitize-recover=all` : toute erreur
d'adressage ou tout comportement indéfini interrompt la cible. Le binaire CLI
instrumenté (`build/asan/bin/moon_duration`) est celui que lance
`test_integration`, de sorte que les exécutions de bout en bout sont vérifiées
elles aussi.

`tests/fuzz/fuzz_smoke.c` n'utilise aucune bibliothèque externe : un générateur
xorshift produit des entrées variées (dates hors bornes, décalages extrêmes,
valeurs non finies, arguments CLI plausibles et piégés) soumises à ASan/UBSan.
Les modules d'orientation du limbe y sont exercés avec des Julian Day et des
positions aléatoires, y compris hors plage. Le balayage d'événements et le calcul
de durée sont également exercés, aux pôles comme sur des fenêtres aléatoires.
La graine par défaut est fixe, donc un échec est reproductible avec `FUZZ_SEED`.

La page de démonstration web est testée séparément (4 tests) :

```bash
make test_web_pages
```

`tests/test_web_pages.c` vérifie le contrat statique de `web/index.html` et de
`web/js/app.js` : politique de sécurité autorisant le chargement local du module
WebAssembly, absence de script ou de style en ligne, point d'entrée de module
important `moon_core.mjs`, et présence du champ d'heure, du bouton `Maintenant`
et des exports `moon_web_*_at`. Ces assertions verrouillent la classe de régression
où une directive CSP trop stricte empêche le chargement du binaire, ou où l'aperçu
daté disparaît de la page.

## Référence de précision

Les heures de référence proviennent des tables RTS (*Rise-Transit-Set*) de
[NASA/JPL Horizons](https://ssd.jpl.nasa.gov/horizons/), avec les paramètres
`EPHEM_TYPE=OBSERVER`, `CENTER=coord@399`, `STEP_SIZE='1m TVH'`,
`R_T_S_ONLY=YES` et `TIME_DIGITS='SECONDS'`. Les valeurs sont figées dans
`tests/test_moon_duration_reference.c` pour garantir des tests reproductibles
sans accès réseau.

Le test `test_reference_precision_matches_jpl_horizons` calcule l'erreur absolue
moyenne et l'erreur maximale sur 110 événements (55 cas répartis sur plusieurs
villes, saisons et années de 1900 à 3000). Les seuils vérifiés sont 45 secondes
pour la moyenne et 80 secondes pour le maximum. La tolérance absolue par
événement est de `0,0009` jour (environ 78 secondes).

## Couverture de code

```bash
make coverage
```

Compile les sources avec `gcov`, exécute les tests puis affiche un tableau
lisible avec la couverture lignes, branches et branches prises par fichier. Les
rapports détaillés sont générés dans `coverage/report/`.

Couverture actuelle :

- lignes : environ 95,7 % ;
- branches exécutées : environ 99,7 % ;
- branches prises : environ 81,4 %.

Les chemins restants sont principalement des erreurs défensives du système
(`localtime_r`, `gmtime_r`, `mktime`), des échecs d'allocation ou de fichier et
des branches défensives non accessibles avec des entrées valides.

### Seuil de couverture

```bash
make coverage-check
```

Échoue si la couverture globale passe sous les seuils, exprimés en pourcentage
et surchargeables :

```bash
make coverage-check COVERAGE_MIN_LINES=95 COVERAGE_MIN_BRANCHES=80
```

Les valeurs par défaut sont `90` pour les lignes et `75` pour les branches
prises. La mesure des branches utilise « prises au moins une fois », seule
métrique qui distingue les deux directions d'un branchement. La CI exécute
`make coverage` puis `make coverage-check` dans un emploi dédié, avec `gcc`
(`clang` ne fournit pas de rapports `gcov`).

## Règles d'écriture des tests

- Chaque test vérifie un comportement observable.
- Séparer le cas nominal, les cas limites et les erreurs.
- Plusieurs assertions sont acceptables si elles vérifient ensemble le même
  comportement.
- Créer un contexte mutable neuf pour chaque test.
- Toute modification de code source doit s'accompagner de tests unitaires
  couvrant le comportement ajouté ou modifié.
