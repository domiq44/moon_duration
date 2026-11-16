# Guide de contribution

## Consignes pour les tests

**Structure AAA.** Chaque test suit AAA, avec des sections `Given`, `When` et
`Then`.

**Scénarios.** Garder chaque test centré sur un comportement observable.
Séparer le cas nominal, les cas limites et les erreurs.

**Assertions.** Plusieurs assertions sont acceptables si elles vérifient
ensemble le même comportement. Éviter de regrouper des vérifications sans lien.

**Noms et descriptions.** Nommer chaque test selon le comportement attendu.
Précéder chaque fonction de test d'une brève description du comportement
vérifié. Ne pas ajouter ces descriptions aux helpers.

**Given.** Déclarer les entrées, les résultats attendus et le contexte du
scénario. Rendre les résultats attendus `const` lorsque c'est approprié, puis
les réutiliser dans `Then`.

**État des tests.** Créer un contexte mutable neuf pour chaque test. Les
fixtures immuables peuvent être partagées. Ne pas dépendre de l'ordre
d'exécution ni de l'état laissé par un autre test.

**Préparation.** Garder les préparations propres à un scénario dans son
`Given`. Factoriser dans un helper `setup_*` seulement si cela réduit une
duplication ou clarifie le test. Appeler ce helper depuis le `Given` de chaque
test concerné. Il doit créer un état propre sans exécuter le comportement testé.

**Organisation.** Séparer helpers, cas de test et runner (`main`). Placer les
helpers avant les cas. Réserver le préfixe `test_` aux fonctions de test. Le
runner n'enregistre que les cas, jamais les helpers.

**Langue.** Rédiger les commentaires des tests dans la langue du projet. Les
balises AAA `Given`, `When` et `Then` peuvent rester en anglais.

**Helpers.** Choisir un nom qui décrit leur rôle (`setup_*`, `assert_*`,
`capture_*`, etc.). Les garder dans le fichier de test tant qu'ils ne servent
qu'à celui-ci; les partager seulement s'ils servent plusieurs suites.

**When.** Effectuer l'action principale, en limitant la préparation et les
actions sans rapport.

**Then.** Vérifier les résultats observables et fournir un diagnostic utile
en cas d'échec.

**Runner.** Déclarer les cas dans un tableau et déléguer l'exécution, le
reporting et le bilan au runner partagé. Utiliser les statuts `PASS`, `FAIL` et
`SKIP`. Le runner identifie chaque cas, poursuit après un échec et calcule les
totaux sans compteur manuel dans la suite.

## Conception et responsabilités

**Principe de responsabilité unique (SRP).** Chaque module doit avoir une seule
raison de changer. Séparer les responsabilités distinctes dans des fonctions ou
des modules dédiés plutôt que de les regrouper dans une fonction fourre-tout.

**Rendu.** Le module de rendu est responsable de son format (ASCII, texte ou
autre). Les calculs métier, la logique de rendu et le formatage du rapport
doivent rester séparés.

**Sortie CLI.** Le rapport affiche les informations textuelles puis, le cas
échéant, un rendu. Le rendu doit être déterministe, borné par des constantes de
dimensions, et chaque ligne doit être terminée conformément au format de sortie
(par exemple un caractère nul en C).

**Tests du rendu.** Les tests doivent vérifier la forme, la position des zones,
les cas limites (rendu vide, plein, etc.) et le comportement avec une sortie
nulle.

**Couverture des modifications.** Toute modification de code source doit
s'accompagner de tests unitaires couvrant le comportement ajouté ou modifié.
Aucune modification ne doit être intégrée sans au moins un test qui l'exerce.

**Internationalisation (i18n).** Les chaînes de l'interface sont localisées via
un module dédié. La sélection se fait par variable d'environnement, puis locale
système, ou par option CLI. La langue par défaut doit être documentée. Toute
nouvelle chaîne utilisateur doit être ajoutée aux tables de traduction et
couverte par un test.

**Documentation.** Toute modification de fonctionnalité, d'option CLI, de
variable d'environnement ou de cible Makefile doit s'accompagner d'une mise à
jour du document propriétaire et, le cas échéant, de `AGENTS.md`. La
documentation doit toujours refléter l'état actuel du code.

**Source unique (SRP appliqué aux documents).** Une information ne vit que dans
un seul document, celui qui répond à sa question. Les autres la référencent par
un lien, jamais en la recopiant. Une modification met à jour le document
propriétaire, pas tous ceux qui en parlent. Un compteur, une table d'options ou
un exemple de sortie ne doit apparaître qu'une seule fois.

## Vérifications à exécuter

Avant de proposer une modification, lancer :

```bash
make test              # suite C complète (inclut check-test-sources)
make test_sanitize     # suites + fuzzing sous ASan/UBSan
make test_web_wasm     # module WebAssembly (nécessite emcc)
make lint              # clang-tidy
make cppcheck          # cppcheck
make format-check      # style clang-format, sans réécrire
make coverage          # rapport de couverture
make coverage-check    # seuils de couverture (90 % lignes, 75 % branches)
```

`make test` commence par `make check-test-sources`, qui échoue si un fichier
`tests/test_*.c` n'est pas déclaré dans `TEST_SRCS`. Tout nouveau fichier de test
doit donc être ajouté à cette liste du `Makefile`.

`make test_sanitize` recompile les mêmes suites dans `build/asan/` avec
`-fsanitize=address,undefined -fno-sanitize-recover=all` (échec sur la première
erreur), y compris le binaire CLI instrumenté qu'utilise `test_integration`,
puis exécute le harnais `tests/fuzz/fuzz_smoke.c`. Le harnais tire des entrées pseudo-aléatoires
(`FUZZ_SEED` pour reproduire un échec, `FUZZ_ITERS` pour le nombre de tours) et
couvre `cli_parse`, les dates, les phases, les événements et les exports web. Un
nouveau module exposé doit y être ajouté.

Le style de formatage est défini par `.clang-format` : `make format` l'applique,
`make format-check` échoue si un fichier n'est pas conforme. Le CI exécute
toutes ces cibles.
