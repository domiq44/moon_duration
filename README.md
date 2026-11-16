# Moon Duration Calculator

Programme C qui calcule le lever, le coucher et l'intervalle de visibilité de
la Lune pour une date et un lieu donnés. La durée affichée est l'intervalle
complet entre le lever et le coucher qui chevauche la journée locale : le
coucher peut donc avoir lieu après minuit. Il s'agit de l'intervalle complet, et
non de la somme des tranches passées au-dessus de l'horizon dans la journée.

## Démo

Une démo interactive, construite sur le même noyau C compilé en WebAssembly,
est disponible sur
[GitHub Pages](https://domiq44.github.io/moon_duration/).

## Démarrage rapide

```bash
make
./build/bin/moon_duration 2026-11-14
```

Le rapport affiche la date et le jour de semaine, le lieu, l'instant de
l'aperçu, l'heure locale et UTC du lever et du coucher, la durée, la phase,
l'illumination, l'inclinaison du limbe et un dessin ASCII orienté par cette
inclinaison. La sortie complète est documentée dans
[`docs/UTILISATION.md`](docs/UTILISATION.md).

## Documentation

| Document | Question |
| ---------- | ---------- |
| [`docs/INSTALLATION.md`](docs/INSTALLATION.md) | Comment compiler, installer et désinstaller ? |
| [`docs/UTILISATION.md`](docs/UTILISATION.md) | Comment utiliser le CLI et la page web ? |
| [`docs/DESIGN.md`](docs/DESIGN.md) | Comment le projet est-il structuré et pourquoi ? |
| [`docs/ALGORITHME.md`](docs/ALGORITHME.md) | Comment la Lune est-elle calculée, avec quelle précision ? |
| [`docs/I18N.md`](docs/I18N.md) | Comment ajouter une langue ou une chaîne ? |
| [`docs/TESTS.md`](docs/TESTS.md) | Comment vérifier, mesurer la couverture et fuzzzer ? |

Une information ne vit que dans un seul document : les autres la référencent par
un lien.

## Licence

Distribué sous licence MIT. Voir le fichier [LICENSE](LICENSE).
