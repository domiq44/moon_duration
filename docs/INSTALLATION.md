# Installation

## Prérequis

- un compilateur C99 : `gcc` ou `clang` ;
- `make` ;
- la bibliothèque mathématique (`libm`).

Le compilateur se choisit avec `CC` :

```bash
make CC=clang
```

## Compilation

```bash
make
```

L'exécutable est généré dans `build/bin/moon_duration`.

## Page web

La démonstration web nécessite Emscripten (`emcc`) et Node.js. Le module
WebAssembly se compile ainsi :

```bash
make web_wasm
```

Le lancement du serveur local et les cibles de test sont décrits dans
[`UTILISATION.md`](UTILISATION.md) et [`TESTS.md`](TESTS.md).

## Installation système

```bash
make install
```

Par défaut, le binaire est installé dans `/usr/local/bin`. Les préfixes peuvent
être personnalisés :

```bash
make install PREFIX=$HOME/.local
make install DESTDIR=/tmp/stage PREFIX=/usr
```

## Désinstallation

```bash
make uninstall
```

Avec un préfixe personnalisé :

```bash
make uninstall PREFIX=$HOME/.local
```

## Intégration continue

Le CI exécute la suite complète avec `gcc` puis avec `clang`, ainsi que
`clang-tidy`, `cppcheck` et `format-check`. La compilation depuis une archive
sans dépôt Git est couverte par la valeur de repli de version.

## Nettoyage

```bash
make clean
```

Supprime `build/`, `coverage/`, ainsi que le module WebAssembly (`web/js/moon_core.mjs`
et `web/js/moon_core.wasm`).

## Artefacts générés

Ces fichiers sont ignorés par Git et reconstruits par `make` :

- `build/` : objets et exécutables ;
- `build/asan/` : objets et exécutables instrumentés (ASan/UBSan) ;
- `coverage/` : objets instrumentés, exécutions de test et rapports gcov ;
- `web/js/moon_core.mjs` et `web/js/moon_core.wasm` : noyau WebAssembly généré.

Pour vérifier l'installation, voir [`TESTS.md`](TESTS.md).
