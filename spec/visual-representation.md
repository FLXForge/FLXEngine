# Visual Representation

## Estado
- Documento: especificación normativa
- Ámbito: representación visual declarativa de objetos
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Principio

Un `RuntimeObject` puede tener posición, tamaño espacial y una representación visual.

La representación visual describe cómo se dibuja el objeto. No define identidad, colisión, movimiento ni existencia runtime.

`shape` queda retirado en v0.3.0. No existe compatibilidad declarativa con `shape`, `shape.type`, `radius`, `content`, `layer` ni `depth` root-level.

## 2. Estructura

```json
{
  "origin": { "x": 160, "y": 90 },
  "size": { "width": 40, "height": 20 },
  "visual": {
    "color": "white",
    "depth": 0,
    "representation": [
      { "primitive": "rectangle" }
    ]
  }
}
```

`size` pertenece al objeto y representa su extensión espacial viva. Puede existir aunque el objeto no tenga `visual`.

`visual.color` es el color base visual. Su ausencia es distinta de declarar un color concreto.

`visual.depth` controla únicamente el orden de dibujo del objeto.

`visual.representation` es una lista ordenada de elementos visuales locales al pivot del objeto.

## 3. Familias de representación

Cada elemento de `representation` debe declarar exactamente una familia:

```text
primitive
geometry
text
```

No se mezclan familias dentro del mismo elemento.

## 4. Primitive

```json
{
  "primitive": "rectangle",
  "mode": "fill",
  "size": { "width": "100%", "height": 8 }
}
```

Valores válidos:

```text
rectangle
triangle
ellipse
```

Todos los primitives siguen el espacio local del objeto, incluido `RuntimeObject.angle`.

`mode` puede ser:

```text
fill
outline
```

Si una dimensión de `size` no se declara, hereda la dimensión correspondiente de `RuntimeObject.size`.

Cada dimensión puede ser un número lógico o un porcentaje sobre la dimensión correspondiente de `RuntimeObject.size`.

Para resolver una dimensión por herencia o porcentaje, el objeto debe tener `size` declarado o generado en runtime mediante `resize*`.

## 5. Geometry

```json
{
  "geometry": [
    { "x": -8, "y": 8 },
    { "x": 0, "y": -8 },
    { "x": 8, "y": 8 }
  ],
  "mode": "fill"
}
```

Los puntos son locales al pivot del objeto.

Una geometría puede declarar desde un punto. Un punto se dibuja como una celda lógica; `close` y `fill` con un único punto no crean área.

`mode` puede ser:

```text
open
close
fill
```

`fill` usa regla par-impar para representar polígonos cóncavos de forma estable.

## 6. Text

```json
{
  "text": "ARKANOID",
  "fontSize": 16
}
```

`fontSize` es obligatorio.

Los saltos explícitos `\n` crean texto multiline. El bloque completo se centra respecto al pivot y cada línea se centra horizontalmente sobre el mismo eje local X.

El texto declarativo es representación del mundo. No sustituye a `draw_text()`, que sigue siendo dibujo inmediato desde JavaScript.

## 7. Color

Un elemento puede declarar su propio `color`. Si no lo declara, usa `visual.color`. Si `visual.color` también está ausente, el renderer usa el color efectivo por defecto.

La proyección del color por Video Chip se aplica durante compilación, no como filtro por frame.

Desde JavaScript:

```js
apply_color(object, "red");
restore_color(object);
```

`apply_color` cambia el color visual runtime del objeto. `restore_color` recupera el color declarado al crear la instancia, incluida la ausencia de color declarado.

## 8. Invariantes

- VIS-001 `shape` no forma parte de FLX v0.3.0.
- VIS-002 `size` pertenece al objeto, no a `visual`.
- VIS-003 `visual.color` distingue ausencia de valor declarado.
- VIS-004 `visual.depth` controla solo orden de dibujo.
- VIS-005 `representation` conserva el orden declarado.
- VIS-006 Cada elemento declara exactamente una familia.
- VIS-007 Primitive se limita a `rectangle`, `triangle` y `ellipse`.
- VIS-008 Geometry usa puntos locales al pivot.
- VIS-009 Geometry fill usa regla par-impar.
- VIS-010 Text requiere `fontSize`.
