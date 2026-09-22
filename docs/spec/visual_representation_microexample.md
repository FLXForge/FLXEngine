# Visual Representation — microejemplo normativo

Acompaña a `examples/visual_representation_micro` y mantiene una comprobación visual pequeña de contratos difíciles de aislar en juegos completos.

## Primitive

Una Primitive puede heredar dimensiones del RuntimeObject, sobrescribir un eje o expresar porcentajes. Una Ellipse no circular rotada permite comprobar que `RuntimeObject.angle` se aplica antes de la presentación.

## Geometry

```json
{ "geometry": [ { "x": 0, "y": 0 } ] }
```

es válida. `open`, `close` y `fill` expresan path abierto, cierre y relleno. Una geometría auto-intersectada sigue siendo válida y `fill` determina su interior mediante even-odd.

Geometry no tiene `size` y `resize(object)` no escala sus puntos.

## Text

```json
{ "text": "LONG\nX", "fontSize": 12 }
```

debe centrar ambas líneas sobre el mismo eje. El bloque completo rota alrededor del pivot. `fontSize` pertenece al espacio lógico; la forma concreta de los glyphs de la fuente default no constituye una garantía geométrica de Representation.

## Composición

Una Representation puede mezclar Primitive, Geometry y Text. Todos son elementos declarativos de un único RuntimeObject; el array determina orden interno, no identidades runtime.

## Qué protege

El microejemplo detecta visualmente regresiones en rotación de Ellipse, dimensiones absolutas/heredadas/porcentuales, Geometry de un punto, open/close/fill, even-odd, multiline, composición, color base/override, depth y separación Representation/Drawing procedural.

Complementa los tests automáticos y los juegos; no los sustituye.
