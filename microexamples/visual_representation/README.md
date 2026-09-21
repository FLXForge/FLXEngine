# Visual Representation v0.3 — micro smoke

Este ejemplo es un smoke visual mínimo, no una demo de gameplay.

Comprueba: Rectangle, Triangle y Ellipse rotada; herencia de tamaño por eje; porcentajes; Geometry de 1 punto y modos open/close/fill; fill even-odd auto-intersectado; Text single/multiline/rotated; composición de elementos; color base/override; depth; y Drawing procedural Local/World.

Resultado esperado: no debe existir clipping implícito por `RuntimeObject.size`; Geometry y Text no redefinen el extent; la elipse alargada debe rotar; `LONG` y `X` deben compartir centro horizontal; el RuntimeObject de mayor depth debe aparecer delante.

La apariencia concreta de los glyphs depende de la fuente/rasterización actual y no forma parte del contrato Visual Representation v0.3.
