# Visual Representation v0.3 — cierre de auditoría

Estado: **CLOSED**

## Contrato final

`RuntimeObject.size` es un **extent espacial opcional**, no tamaño visual ni de colisión. Su ausencia es distinta de `0×0` explícito. Visual, Collision y otras capacidades pueden consumir estado del RuntimeObject cuando su contrato lo permita, pero su geometría nunca redefine retroactivamente ese estado.

```text
RuntimeObject
├─ position / pivot
├─ size?                 optional spatial extent
├─ visual?
├─ collider(s)?
├─ mechanics?
├─ children?
└─ scripts / lifecycle
```

`visual` declara estado visual base y puede contener una `representation` persistente. No es un canvas y no tiene `size`. Un RuntimeObject puede no declarar Visual y seguir siendo válido o producir Drawing procedural.

Una Representation es un array ordenado de RepresentationElements. Los elementos no son RuntimeObjects y no poseen identidad, lifecycle, scripts, children, mechanics, collider, position, angle, visibility ni depth propios. Comparten el pivot, angle, visibility, depth y presentación/wrap del RuntimeObject.

Familias v0.3: `primitive`, `geometry`, `text`. Tras resolver `like`, cada elemento efectivo debe pertenecer exactamente a una familia.

## Primitive

Tipos: `rectangle`, `triangle`, `ellipse`. Square es Rectangle con dimensiones iguales; Circle es Ellipse con dimensiones iguales.

Cada eje puede ser absoluto, porcentaje del eje correspondiente de `RuntimeObject.size`, u omitirse para heredarlo. Tras resolver dependencias deben existir width y height efectivos.

`mode = fill | outline`, default `fill`. Todas las primitivas siguen `RuntimeObject.angle`.

## Geometry

Geometry usa puntos locales explícitos y no posee `size`. RuntimeObject.size no la escala, recorta, limita ni normaliza.

`mode = open | close | fill`, default `open`. Acepta desde un punto. Se permiten degeneraciones, repetición, colinealidad, backtracking, concavidad y auto-intersección. `fill` usa regla **even-odd**.

## Text

Text se define mediante `text` + `fontSize`. No tiene `size` ni `mode`. `fontSize` está expresado en unidades lógicas; output scale sólo presenta posteriormente el raster.

El bloque se centra respecto al pivot. `\n` crea multiline explícito; cada línea comparte el mismo centro horizontal y el bloque completo rota alrededor del pivot.

No existen en v0.3 word wrapping automático, Text outline, font selection, baseline pública, offsets ni mutación runtime del contenido.

La apariencia concreta de los glyphs de la fuente default pertenece a rasterización/fuente y no al contrato geométrico de Representation.

## Color, depth y visibility

Color efectivo:

```text
element.color
    ??
live visual base color
    ??
Machine/default
```

`apply_color` cambia el base vivo; `restore_color` restaura el declarado o la dependencia al fallback si originalmente estaba ausente.

`visual.depth` default 0. Menor depth se pinta antes; empate estable. JS permanece plano mediante `object.depth` y `depth(object,value)`.

Un RuntimeObject oculto no produce Representation, no ejecuta `draw(object)` ni produce Drawing Local asociado. Drawing World sin RuntimeObject permanece independiente.

## Fronteras

Visual y Collision son independientes. Collider nunca hereda bounds de Primitive, Geometry o Text.

Wrap genera instancias de presentación, no nuevas identidades ni nuevas ejecuciones de `draw(object)`.

El modelo legacy `shape` queda retirado sin alias silencioso.

Representation queda preparada para incorporar mecanismos futuros como Raster/Tile y para que una futura Animation pueda operar sobre Representations completas, sin definir todavía ese contrato.

## Evidencia de cierre

Implementación principal: `7f507463d504c083b04c4b1bb3e7eb0cefcae964`.

Corrective gate: `6f2e5e5a75974a189bfa9654273466f2287403c3`.

Debug/Release y CTest 23/23 verdes; smokes source y `.flxc` de Pong, Arkanoid, Asteroids e Invaders verdes; smoke visual humano correcto. Las peculiaridades observadas en glyphs de Asteroids no muestran una violación del layout de Visual Representation.

El microejemplo `visual_representation_micro` queda como comprobación visual dirigida y documentación ejecutable.

**Visual Representation v0.3: CLOSED.**
