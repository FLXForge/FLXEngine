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

En cada draw pass, Runtime toma un snapshot de objetos candidatos vivos y visibles, los ordena de forma estable por `visual.depth` y procesa cada objeto todavía vivo y visible:

```text
visual.representation declarativa
→ draw(object) JS
→ resolución de presentación
→ render
```

Un objeto anterior puede ocultar o matar un objeto posterior antes de su turno visual.

## 3. Depth

`depth` es una propiedad de `visual`.

```json
{
  "visual": {
    "depth": -10
  }
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

La captura incluye tanto la geometría mundial resultante como la información de presentación necesaria para `wrap`. Una primitiva local ya emitida no vuelve a consultar el estado vivo del objeto de referencia durante el render del mismo pass.

La representación declarativa de un objeto se captura antes de ejecutar `draw(object)`. Si `draw(object)` modifica posición, tamaño, ángulo o presentación del mismo objeto, esa mutación no cambia retroactivamente la representación ya emitida en el pass actual.

## 6. Presentación y wrap

Una primitiva mundial no tiene fuente de presentación y no genera copias por wrap.

Una primitiva local usa el objeto de referencia como fuente de presentación. Si ese objeto usa `bounds.wrap` y está en overflow, la primitiva comparte las mismas instancias de presentación que la representación declarativa de ese objeto.

`draw(object)` se ejecuta una sola vez por objeto, aunque existan copias visuales por wrap.

La presentación se decide desde el snapshot capturado al emitir la primitiva. Dos primitivas emitidas antes y después de mover la misma referencia pueden tener distintas instancias de presentación dentro del mismo hook.

## 7. Primitivas

`draw_pixel(x, y, color?)` dibuja un pixel lógico.

`draw_line(x, y, x1, y1, color?)` dibuja una línea entre dos puntos lógicos.

`draw_rectangle(x, y, width, height, color?)` dibuja únicamente el contorno de un rectángulo. `x/y` representan su centro/pivot.

`draw_text(x, y, text, size?, color?)` dibuja texto centrado en `x/y`.

`size` (por defecto `10`) está expresado en unidades lógicas, igual que el
`fontSize` del texto declarativo. Los dos caminos usan la misma fuente y el mismo
renderer. No se impone un mínimo de 10 u 11: tamaños 6 y 8 siguen siendo válidos.
La proyección mundo/raster y el escalado posterior de salida son etapas distintas;
los bindings no cambian el tamaño en función de `video.output.scale`.

En raylib 6.0 la fuente predeterminada es un bitmap con tamaño base de 10 píxeles
y filtrado por vecino más próximo. Reducirla a 6 u 8 puede omitir filas y columnas
de sus glifos y perder trazos. Bordes definidos no garantizan legibilidad perfecta.
Una posición entera tampoco garantiza que todas las posiciones de glifos sean
enteras: el centrado, sus anchuras y el factor `size / 10` pueden producir fracciones.
El renderer conserva esas métricas y la rotación; no redondea posiciones para
intentar compensar la pérdida de información de la fuente.

El mundo se dibuja en el render target lógico de Machine. Con
`video.output.smoothing=false`, la presentación a escala entera repite sus píxeles
sin mezclarlos; no puede recuperar trazos ya perdidos en ese raster. Con
`smoothing=true`, el filtrado bilineal de la pantalla puede suavizar bordes. Este
filtro es distinto del filtro de la textura de la fuente.

Todas aceptan overload local con `RuntimeObject` como primer argumento.

Los outlines se expresan en unidades lógicas y el renderer los proyecta según `screen.scale`; no quedan limitados accidentalmente a un único píxel físico cuando la escala de salida es mayor que `1`.

## 8. Proyección física

El scripting emite coordenadas lógicas. La conversión a coordenadas físicas/display pertenece al renderer de Drawing.

Los bindings JS no multiplican por `screen.scale`.

## 9. Fade

`fade_on`, `fade_off`, `fade_set`, `fade_active`, `fade_done` y `fade_alpha` pertenecen al overlay global de pantalla.

Fade no es parte de Immediate Drawing, no tiene `depth` y se dibuja por encima del mundo y del dibujo inmediato.

## 10. Invariantes

- DRAW-001 `draw_*` solo produce salida dentro de `draw(object)`.
- DRAW-002 `visual.representation` declarativa se emite antes que el dibujo inmediato del mismo objeto.
- DRAW-003 El orden visual es estable por `depth`.
- DRAW-004 Igual `depth` conserva orden de creación/carga.
- DRAW-005 `depth(object,value)` no reordena el pass ya iniciado.
- DRAW-006 Primitivas mundiales no usan presentación por wrap.
- DRAW-007 Primitivas locales usan la presentación del objeto de referencia.
- DRAW-008 La referencia de coordenadas no cambia el propietario visual.
- DRAW-009 Los bindings JS no realizan escalado físico.
- DRAW-010 Fade es overlay global independiente de Drawing.
- DRAW-011 Representación declarativa y primitivas locales capturan presentación al emitirse.
- DRAW-012 Mutaciones posteriores durante el mismo hook no alteran primitivas ya emitidas.

## 11. Caracterización de texto pequeño

`flx-text-rasterization-tests` requiere una sesión con OpenGL. Compara píxeles
para tamaños 6, 8, 10, 11 y 12; posiciones enteras y cuartos de píxel; una y varias
líneas, incluyendo líneas vacías; rotaciones 0, 25 y 90 grados; raster de 240x80 y
320x80; texto declarativo y local inmediato. Comprueba por separado la fuente,
el raster lógico y la presentación a escalas 1, 2 y 3, con y sin smoothing.
`RuntimeDrawingTests` también verifica ambos overloads JavaScript sin dispositivo
gráfico: tamaño lógico, coordenadas fraccionarias, rotación local y defaults.

La caracterización de v0.3.1 confirma que la fuente default ya equivale a POINT
y que el texto blanco sobre negro no tiene píxeles grises en el raster lógico.
La presentación POINT a escala entera reproduce exactamente esos píxeles.
Para `MWil1 0123456789`, el modelo de muestreo con origen superior entero deja
sin muestrear 45 filas nativas con tinta a tamaño 6 y 29 a tamaño 8 (sumadas por
glifo), frente a cero a tamaño 10. Son filas de la fuente omitidas por muestreo,
no una puntuación de legibilidad ni un recuento de trazos completos desaparecidos.

El origen vertical actual de cada línea es la mitad de `lineHeight = size + 1`
a escala mundo/raster 1; se conserva el espaciado existente. Para una línea,
el borde superior nominal está en `y - (size + 1)/2`. Usar alternativamente
`MeasureTextEx().y/2` desplaza el muestreo medio píxel, pero no recupera las filas
omitidas por reducir los glifos. La prueba genera esa alternativa como comparación,
sin cambiar el renderer ni afirmar que mejore consistentemente la nitidez.

No se ha encontrado en esta auditoría un fallo de filtrado que explique la pérdida
de detalle observada. Los resultados no justifican cambiar filtros ni redondear coordenadas.
El siguiente paso para buscar una mejora consistente sería evaluar glifos
diseñados para esos tamaños; sustituir la fuente o introducir un sistema
tipográfico queda fuera de esta corrección. La legibilidad óptica de cada cadena no queda
demostrada únicamente por estos recuentos de píxeles.

Para reproducir, compilar el motor y ejecutar `flx-text-rasterization-tests` y
`flx-runtime-drawing-tests` sin `FLX_TEST_HEADLESS=1`. La primera prueba devuelve
77 (skip en CTest) si se solicita ejecución headless o no se puede abrir ventana.
Opcionalmente, definir `FLX_TEXT_ARTIFACT_DIR` con la ruta absoluta de
`out/text-characterization`: exporta `centering-before-candidate.png`, ampliada
3x con vecino más próximo, con el renderer actual a la izquierda y el origen
métrico alternativo a la derecha. La imagen es diagnóstica; no es un antes/después
de una corrección aplicada. No se necesitan imágenes de referencia versionadas.
