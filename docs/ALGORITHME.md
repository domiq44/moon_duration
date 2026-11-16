# Algorithme lunaire

## 1. Position de la Lune

La position géocentrique est calculée à partir des éléments orbitaux moyens de
Paul Schlyter, exprimés en jours depuis l'époque `1999-12-31 00:00 UT`
(`JD 2451543.5`) :

```text
N  = 125.1228 - 0.0529538083 * d      (nœud ascendant)
i  = 5.1454                            (inclinaison)
w  = 318.0634 + 0.1643573223 * d       (argument du périgée)
a  = 60.2666                           (demi-grand axe, rayons terrestres)
e  = 0.054900                          (excentricité)
M  = 115.3654 + 13.0649929509 * d      (anomalie moyenne)
```

L'anomalie excentrique est résolue par une approximation du premier ordre :

```text
E = M + e * sin(M) * (1 + e * cos(M))
```

Les coordonnées dans le plan orbital sont d'abord exprimées en coordonnées
écliptiques. On note `u = v + w` l'argument de latitude et `r` la distance
géocentrique :

```text
xv = a * (cos(E) - e)
yv = a * sqrt(1 - e^2) * sin(E)
v  = atan2(yv, xv)
r  = sqrt(xv^2 + yv^2)
u  = v + w

xh = r * (cos(N) * cos(u) - sin(N) * sin(u) * cos(i))
yh = r * (sin(N) * cos(u) + cos(N) * sin(u) * cos(i))
zh = r * (sin(u) * sin(i))

lon = atan2(yh, xh)
lat = atan2(zh, sqrt(xh^2 + yh^2))
```

`lon` et `lat` sont la longitude et la latitude écliptiques, en degrés. `r`
est exprimé en rayons terrestres.

### Perturbations périodiques

Les perturbations principales viennent de l'attraction gravitationnelle du
Soleil. Elles sont ajoutées à `lon`, `lat` et `r` sous forme de termes
sinusoïdaux. On calcule d'abord :

```text
L_m = M + w + N              (longitude moyenne de la Lune)
L_s = M_s + w_s              (longitude moyenne du Soleil)
D   = L_m - L_s              (élongation moyenne)
F   = L_m - N                (argument de latitude)
```

où `M_s = 356.0470 + 0.9856002585 * d` et `w_s = 282.9404 + 4.70935e-5 * d`
sont l'anomalie moyenne et la longitude du périhélie du Soleil. `L_s` est la
longitude **moyenne** du Soleil (`mean_longitude`), distincte de sa longitude
vraie (section 7.1) : les perturbations lunaires utilisent la longitude
moyenne.

Les termes de longitude sont :

```text
Δlon = -1.274 sin(M - 2D)
       +0.658 sin(2D)
       -0.186 sin(M_s)
       -0.059 sin(2M - 2D)
       -0.057 sin(M - 2D + M_s)
       +0.053 sin(M + 2D)
       +0.046 sin(2D - M_s)
       +0.041 sin(M - M_s)
       -0.035 sin(D)
       -0.031 sin(M + M_s)
       -0.015 sin(2F - 2D)
       +0.011 sin(M - 4D)
```

Les trois premiers termes sont les plus importants :

- **Évection** : `-1.274 sin(M - 2D)`. C'est la plus forte perturbation
  périodique de la longitude lunaire. Elle est due à l'attraction solaire et
  dépend de l'anomalie moyenne de la Lune et de l'élongation.
- **Variation** : `+0.658 sin(2D)`. Elle dépend de l'élongation moyenne `2D` et
  décrit l'oscillation de la longitude liée à la position de la Lune par
  rapport au Soleil.
- **Équation annuelle** : `-0.186 sin(M_s)`. Elle est due à l'excentricité de
  l'orbite terrestre autour du Soleil.

Les autres termes de longitude sont des corrections périodiques de même nature,
avec des amplitudes décroissantes (de `0,059°` à `0,011°`). Ils affinent
l'évection, la variation et l'équation annuelle.

Les termes de latitude sont :

```text
Δlat = -0.173 sin(F - 2D)
       -0.055 sin(M - F - 2D)
       -0.046 sin(M + F - 2D)
       +0.033 sin(F + 2D)
       +0.017 sin(2M + F)
```

Le terme principal est `-0.173 sin(F - 2D)`, la plus forte correction de
latitude due à l'attraction solaire.

Les termes de distance sont :

```text
Δr = -0.58 cos(M - 2D) - 0.46 cos(2D)
```

Ils représentent les variations périodiques de la distance Terre-Lune causées
par la perturbation solaire. `Δr` est exprimé en rayons terrestres.

### Conversion écliptique vers équatoriale

La longitude `lon` et la latitude `lat` sont dans le repère écliptique. Pour
obtenir l'ascension droite `RA` et la déclinaison `Dec`, on convertit d'abord
ces angles en vecteur unitaire :

```text
x = cos(lat) * cos(lon)
y = cos(lat) * sin(lon)
z = sin(lat)
```

Le repère équatorial est incliné de l'obliquité `eps` par rapport au repère
écliptique. On applique une rotation autour de l'axe `x`, qui correspond à la
direction de l'équinoxe vernal et reste commun aux deux repères :

```text
xe = x
ye = y * cos(eps) - z * sin(eps)
ze = y * sin(eps) + z * cos(eps)
```

Puis on revient aux angles :

```text
RA  = atan2(ye, xe)
Dec = asin(ze)
```

`RA` est ramené dans `[0, 360)` degrés ; la fonction `atan2` le place dans le
bon quadrant. `asin` est borné à `[-1, 1]` pour éviter les erreurs d'arrondi.
L'obliquité est calculée avec :

```text
T   = (JD - 2451545.0) / 36525
eps = 23.4392911 - 0.0130042 * T - 1.64e-7 * T^2 + 5.04e-7 * T^3
```

`T` est le temps en siècles juliens depuis l'époque J2000.0. La distance `r`
n'intervient pas dans la conversion des angles ; elle est exprimée en rayons
terrestres.

La distance est exprimée en rayons terrestres puis convertie en kilomètres avec
`1 rayon terrestre = 6378.14 km`.

## 2. Altitude apparente

Pour un observateur à la latitude `φ` et à la longitude `L` :

```text
H   = LST - RA
LST = GMST + L
sin(alt) = sin(φ) * sin(Dec) + cos(φ) * cos(Dec) * cos(H)
alt = asin(sin(alt))
```

`GMST` est le temps sidéral moyen de Greenwich, calculé en degrés à partir du
Julian Day.

## 3. Horizon lunaire

Le lever et le coucher correspondent au passage de l'altitude au voisinage de
l'horizon. L'altitude de référence utilisée est :

```text
MOON_HORIZON_ALTITUDE_DEG = 0.125 degré
```

Cette valeur compense la parallaxe lunaire, la réfraction atmosphérique et le
diamètre apparent du disque. Elle est exprimée pour le centre de la Lune.

## 4. Recherche des événements

`moon_events_scan` parcourt l'intervalle de recherche par pas de 5 minutes
(`1/288` jour). Au voisinage de l'horizon, lorsque l'altitude se trouve à moins
de `1,3°` de l'horizon (`HORIZON_MARGIN_DEG`), le pas se resserre à 30 secondes
pour ne pas manquer un croisement bref. Cette marge couvre la variation maximale
de l'altitude lunaire sur un pas grossier (environ `1,21°` en 5 min). À chaque
changement de signe de `altitude - horizon`, une dichotomie de 60 itérations
affine l'instant exact du passage.

Le type d'événement est déduit du sens du changement :

- passage de négatif à positif : lever ;
- passage de positif à négatif : coucher.

## 5. Calcul de la durée

`moon_duration_compute` recherche les événements sur une fenêtre de deux jours
avant et après la journée locale. Chaque lever est associé au coucher suivant.
L'intervalle retenu est celui dont l'intersection avec la journée locale est la
plus grande. La durée est :

```text
duration = (coucher - lever) * 24 heures
```

Cette approche gère les couchers après minuit et les journées sans lever ou sans
coucher (latitudes polaires). La fenêtre journalière balayée est celle de
l'appelant : 23 h ou 25 h lorsque la journée locale contient un changement
d'heure.

Lorsqu'aucun événement n'est détecté dans la journée, la Lune reste d'un côté de
l'horizon. La classification (`all_day` ou `none`) échantillonne alors
l'altitude lunaire au début, au milieu et à la fin de la journée : la Lune est
déclarée visible toute la journée (24 h) si les trois échantillons sont au-dessus
de l'horizon, absente (0 h) s'ils sont tous en dessous. En cas de désaccord (la
Lune frôle l'horizon), l'altitude de début tranche.

## 6. Phase et illumination

La phase et l'illumination sont dérivées de l'élongation entre la Lune et le
Soleil, calculée à partir des positions écliptiques de la section 1 :

```text
Δλ = λ_lune - λ_soleil                      ramené dans [0, 360)
phase = Δλ / 360

cos(ψ) = cos(β_lune) * cos(Δλ)              (β_soleil = 0)
illumination = (1 - cos(ψ)) / 2
```

`ψ` est l'écart angulaire géocentrique entre les deux astres : il tient compte de la
latitude écliptique de la Lune, si bien qu'une pleine lune géocentrique
n'atteint pas exactement 100 % lorsque la Lune est hors de l'écliptique. La
fraction éclairée vaut `(1 - cos(ψ)) / 2` parce que l'angle Soleil-Lune-Terre
est le complément de l'élongation, le Soleil étant quasi à l'infini.

La phase est évaluée à l'instant de l'aperçu (section 7.6). Les huit phases
principales sont déduites de `phase`.

Lorsque la position n'est pas exploitable, le calcul retombe sur la période
synodique moyenne :

```text
phase = ((JD - 2451550.1) mod 29.530588853) / 29.530588853
illumination = (1 - cos(2 * pi * phase)) / 2
```

Ce repli n'est qu'une sécurité : l'élongation moyenne s'écarte de l'élongation
réelle de plus de 8° (0,66 jour), ce qui décale l'illumination de plusieurs
points (section 8).

## 7. Inclinaison du limbe

L'inclinaison du limbe brillant répond à la question « vers où pointe la partie
éclairée de la Lune ? ». Elle se calcule en deux temps : l'angle de position du
limbe dans le repère géocentrique, puis sa correction pour l'observateur.

### 7.1 Position du Soleil

Le Soleil fournit la direction de l'éclairage. Les éléments orbitaux de
Schlyter donnent, en jours `d` depuis l'époque `2451543.5` :

```text
w = 282.9404 + 4.70935e-5 * d          (argument du périhélie)
M = 356.0470 + 0.9856002585 * d        (anomalie moyenne)
e = 0.016709 - 1.151e-9 * d            (excentricité)
```

La longitude écliptique est la longitude vraie, obtenue par l'équation du centre
(section 1) :

```text
E = M + e * (180 / pi) * sin(M) * (1 + e * cos(M))
v = atan2(sqrt(1 - e^2) * sin(E), cos(E) - e)
L = v + w
```

L'équation du centre vaut au plus `2e` radians, soit 1,915° : l'ignorer
décalerait la longitude solaire de près de 2°, l'illumination de plus de 1,5
point et l'angle du limbe de plusieurs degrés. La longitude `L` est ensuite
convertie en ascension droite et déclinaison par la rotation appliquée à la Lune
(section 1), avec une latitude écliptique nulle.

### 7.2 Angle de position géocentrique

L'angle de position `χ` du milieu du limbe éclairé est mesuré depuis le point
nord du disque lunaire, vers l'est (Meeus, chapitre 48) :

```text
Δα = α_soleil - α_lune
y  = cos(δ_soleil) * sin(Δα)
x  = sin(δ_soleil) * cos(δ_lune) - cos(δ_soleil) * sin(δ_lune) * cos(Δα)
χ  = atan2(y, x)                       ramené dans [0, 360)
```

Le limbe pointe vers le Soleil : `χ` vaut environ 90° à la nouvelle lune et
270° à la pleine lune, deux configurations où la direction devient instable.

### 7.3 Angle parallactique

Pour un observateur à la latitude `φ`, l'angle `q` entre le nord céleste et le
zénith, vu depuis la Lune, vaut :

```text
H = LST + L - α_lune                   ramené dans [-180, 180]
q = atan2(cos(φ) * sin(H), sin(φ) * cos(δ_lune) - cos(φ) * sin(δ_lune) * cos(H))
```

La forme est multipliée par `cos(φ)` plutôt qu'écrite avec `tan(φ)` : elle reste
définie aux pôles, où la tangente diverge.

### 7.4 Inclinaison apparente

L'angle du limbe par rapport à la verticale du lieu est la différence des deux
angles précédents :

```text
χ' = χ - q                             ramené dans [0, 360)
```

`χ'` est compté depuis la verticale (projection du zénith sur le plan du ciel)
vers le limbe brillant, dans le sens des angles de position (nord vers est). La
verticale est la direction du zénith, et non celle du nord : c'est `q` qui
traduit l'écart entre les deux.

Comme la Lune tourne autour de la Terre en 27,3 jours, l'orientation apparente
varie d'environ 12° par jour, plus vite que l'illumination.

### 7.5 Disponibilité

Le limbe n'est identifiable que si le disque n'est ni éborgné ni entièrement
éclairé. L'orientation est donc déclarée indéterminée (NaN) lorsque
l'illumination sort de l'intervalle 2–98 %, en réutilisant `moon_illumination`
(section 6) plutôt qu'un second modèle d'illumination. Les deux bornes sont
symétriques : 2 % pour un croissant trop mince, 98 % pour un disque presque
plein.

### 7.6 Vérification

L'angle apparent est vérifié par un second calcul, indépendant de la formule
`χ - q` : le zénith et le Soleil sont projetés sur le plan tangent à la Lune,
puis l'angle entre les deux projections est mesuré. Les deux méthodes concordent
à mieux que `1e-9` degré sous plusieurs latitudes, dont les pôles.

La valeur est aussi recoupée avec une éphéméride indépendante : les positions
apparentes de JPL Horizons (DE441) et une projection vectorielle de la direction
du Soleil donnent, sur six instants du 14 novembre 2026 à Paris et quatre à
Sydney, un angle du limbe concordant à moins de 0,6° avec `χ'`. L'écart résiduel
provient de la précision de la théorie lunaire abrégée (section 8).

L'instant de référence est celui de l'aperçu, dont la règle est définie dans
[UTILISATION](UTILISATION.md#heures-affichées). Le test compare la valeur à midi
local à une référence figée.

## 8. Précision

### Phase et illumination

La fraction éclairée est comparée à la formule géométrique indépendante
`(1 - cos(ψ)) / 2` construite à partir des positions écliptiques : l'écart est
inférieur à `1e-12`, la seule source d'erreur restante étant la précision de la
théorie lunaire abrégée (environ 0,3° en longitude), soit moins de `0,5` point
d'illumination.

La période synodique moyenne, utilisée auparavant, s'écartait de l'élongation
réelle de plus de 8° : l'illumination différait de plus de 2 points environ une
fois sur deux, de plus de 4 points une fois sur quatre, et jusqu'à 8,2 points
(60,6 % affichés pour 52,4 % réels le 11 novembre 2025). Le libellé de phase
changeait de classe sur 9,3 % des jours (34 jours sur 365 en 2026).

### Lever et coucher

L'algorithme de position est une théorie lunaire abrégée. Il est adapté à un
affichage de lever/coucher à la minute. Les tests de référence comparent les
heures calculées aux tables RTS (*Rise-Transit-Set*) de
[NASA/JPL Horizons](https://ssd.jpl.nasa.gov/horizons/), avec une résolution
d'une minute.

La base de référence couvre 55 cas (110 événements) répartis sur plusieurs
villes (Paris, New York, Sydney, Tromsø, Ushuaia, Quito, Honolulu, Tokyo,
Le Cap), plusieurs saisons et des années extrêmes de 1900 à 3000. L'erreur
absolue moyenne est de 24,5 secondes et l'erreur maximale de 65,6 secondes.
Le test `test_reference_precision_matches_jpl_horizons` vérifie que la moyenne
reste inférieure ou égale à 45 secondes et que le maximum reste inférieur ou
égal à 80 secondes.

### Domaine de validité du modèle

Les éléments orbitaux proviennent de J. Meeus et s'appliquent aux époques
proches de J2000. Les arguments moyens (longitudes, anomalie, nœud) et leurs
termes périodiques sont développés au premier ordre en temps : ils restent
valables de 1900 à 3000 avec une erreur qui croît lentement, de l'ordre de
quelques secondes de temps par siècle pour les événements lunaires. Hors de
cet intervalle — dates très anciennes ou très futures, calendrier non
grégorien — les résultats ne sont plus garantis : le CLI émet un avertissement
sur `stderr` et poursuit le calcul, tandis que la page web restreint la date à
1900..3000. La précision annoncée au §Lever et coucher ne s'applique donc qu'à
ce domaine.

## Références

- Paul Schlyter, *Computing planetary positions*,
  <https://www.stjarnhimlen.se/comp/ppcomp.html>
- Jean Meeus, *Astronomical Algorithms*, 2e édition, Willmann-Bell, 1998
  (chapitres 22 et 48).
- NASA/JPL Horizons,
  <https://ssd.jpl.nasa.gov/horizons/>
