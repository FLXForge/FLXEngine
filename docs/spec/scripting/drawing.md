# Drawing JS

## Estado
- Documento: especificación normativa
- Ámbito: dibujo inmediato JavaScript y orden visual de objetos
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Principio

Las funciones `draw_*` son dibujo inmediato asociado al turno visual actual de un `RuntimeObject`.

No crean `RuntimeObject`, no participan en Collision y no se ejecutan como una capa final separada.

## 2. Turno visual

En cada draw pass, Runtime toma un snapshot de objetos candidatos vivos y visibles, los ordena de forma estable por `depth` y procesa cada objeto todavía vivo y visible:

```text
shape declarativa
→ draw(object) JS
→ resolución de presentación
→ render
```

Un objeto anterior puede ocultar o matar un objeto posterior antes de su turno visual.

## 3. Depth

`depth` es una propiedad root-level del objeto.

```json
{
  "depth": -10
}
```

Reglas:

- valor entero;
- default `0`;
- menor `depth` dibuja antes;
- mayor `depth` dibuja después;
- igual `depth` conserva orden de creación/carga;
- no afecta a action, motion, collision, bounds ni input.

La vista JS expone `object.depth` como lectura viva y temporal. La mutación se hace con:

```js
depth(object, value);
```

El nuevo valor se observa inmediatamente en el mismo hook. El orden del draw pass actual se fija al inicio del pass y el cambio afecta al siguiente pass.

`layer` no forma parte de v0.3.0.

## 4. Espacios

Por defecto, `draw_*` usa coordenadas mundiales lógicas FLX.

Si el primer argumento es un `RuntimeObject`, las coordenadas son locales a ese objeto de referencia:

```js
draw_pixel(10, 20, "white");
draw_pixel(ship, 0, -6, "white");
```

El primer `RuntimeObject` de una llamada `draw_*` es referencia de coordenadas, no propietario visual. El propietario visual siempre es el objeto cuyo `draw(object)` se está ejecutando.

## 5. Transformación local

La transformación local usa:

```text
posición/pivot del RuntimeObject de referencia
ángulo del RuntimeObject de referencia
```

`width` y `height` no escalan primitivas locales.

Las primitivas locales capturan su transformación al emitirse. Si el objeto de referencia se mueve después dentro del mismo hook, las primitivas ya emitidas no se mueven retroactivamente.

## 6. Presentación y wrap

Una primitiva mundial no tiene fuente de presentación y no genera copias por wrap.

Una primitiva local usa el objeto de referencia como fuente de presentación. Si ese objeto usa `bounds.wrap` y está en overflow, la primitiva comparte las mismas instancias de presentación que la shape de ese objeto.

`draw(object)` se ejecuta una sola vez por objeto, aunque existan copias visuales por wrap.

## 7. Primitivas

`draw_pixel(x, y, color?)` dibuja un pixel lógico.

`draw_line(x, y, x1, y1, color?)` dibuja una línea entre dos puntos lógicos.

`draw_rectangle(x, y, width, height, color?)` dibuja únicamente el contorno de un rectángulo. `x/y` representan su centro/pivot.

`draw_text(x, y, text, size?, color?)` dibuja texto centrado en `x/y`.

Todas aceptan overload local con `RuntimeObject` como primer argumento.

## 8. Proyección física

El scripting emite coordenadas lógicas. La conversión a coordenadas físicas/display pertenece al renderer de Drawing.

Los bindings JS no multiplican por `screen.scale`.

## 9. Fade

`fade_on`, `fade_off`, `fade_set`, `fade_active`, `fade_done` y `fade_alpha` pertenecen al overlay global de pantalla.

Fade no es parte de Immediate Drawing, no tiene `depth` y se dibuja por encima del mundo y del dibujo inmediato.

## 10. Invariantes

- DRAW-001 `draw_*` solo produce salida dentro de `draw(object)`.
- DRAW-002 Shape declarativa se emite antes que el dibujo inmediato del mismo objeto.
- DRAW-003 El orden visual es estable por `depth`.
- DRAW-004 Igual `depth` conserva orden de creación/carga.
- DRAW-005 `depth(object,value)` no reordena el pass ya iniciado.
- DRAW-006 Primitivas mundiales no usan presentación por wrap.
- DRAW-007 Primitivas locales usan la presentación del objeto de referencia.
- DRAW-008 La referencia de coordenadas no cambia el propietario visual.
- DRAW-009 Los bindings JS no realizan escalado físico.
- DRAW-010 Fade es overlay global independiente de Drawing.
