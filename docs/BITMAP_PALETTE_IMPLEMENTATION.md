# Paletas y Bitmap — implementación y validación v0.4.0

Fecha de revisión: 2026-11-17. Rama: `feature/v0.4.0`.
Cambios locales para revisión; no se han realizado commits, push ni publicaciones.

## Decisiones acordadas

- El motor sigue siendo 0.4.0. El writer `.flxc` pasa a 13 y el reader acepta 12 y 13.
- El tamaño del bitmap hereda el del objeto. Sin `size` espacial, la matriz aporta
  su tamaño natural; el elemento admite overrides numéricos y porcentuales.
- La paleta declarativa aporta nombres; no limita el conjunto de colores. Machine
  conserva sus capacidades actuales y procesa todos los píxeles, incluido `blank`.

## Arquitectura e implementación

`ColorParser::tryParse` reutiliza el parser nativo sin logging ni fallback.
El comportamiento anterior de `parse` se conserva para sus otros consumidores;
la validación estricta se aplica a las declaraciones visuales JSON.

`JsonLoader` y su helper privado `VisualDeclarations.inl` reutilizan el resolver,
su caché, raíces, miembros y diagnósticos. La resolución de mapas de datos no
interpreta claves de paleta como `like` o `visual` como bloques de objeto.
Las composiciones se normalizan en orden, sin modificar referencias compartidas.
Las paletas heredadas se normalizan antes del merge para conservar procedencia y
claves base, también al heredar desde un miembro lógico.

`BitmapDefinition` conserva dimensiones y píxeles normalizados de solo lectura.
`ProjectCompiler` proyecta cada color mediante `VideoColorProcessor`, que no se
modifica. No se añaden paletas runtime, bindings JavaScript ni dependencias.

`RepresentationPrimitiveBuilder` conserva orden y snapshot de posición/tamaño/ángulo.
`DrawingRenderer` dibuja un quad texturizado con filtro puntual, rotado en el mundo
antes de la escala raster. `BitmapTextureCache`, propiedad del mundo, reutiliza la
textura entre frames; no es un sistema público de assets. El cache caliente no
recorre todas las texturas. Se conservan las firmas existentes de render.

El codec 13 añade etiqueta 3 y payload RGBA acotado. La longitud se comprueba antes
de reservar memoria y comparte el presupuesto de bytes existente. El validator
rechaza dimensiones/píxeles inconsistentes. No se amplían los límites globales
para permitir 255x255. El fixture Windows del formato 12 y su SHA-256 se conservan.

## Especificaciones y documentación

- Nuevo contrato formal: `spec/visual-bitmap-palette.md`.
- Contratos actualizados: `spec/visual-representation.md`, `references_v2.md`,
  `compiled-project.md`, `compiled-binary.md`, `diagnostics.md` e índice `README.md`.
- Se corrigen las declaraciones de formato vigente inconsistentes; se conservan
  versiones iniciales e información histórica.
- Schema: `docs/schemas/visual.schema.json`, con gramática de paletas, filas,
  patrones no recursivos y repeticiones. Los alias contextuales y las dimensiones
  expandidas se validan en el compilador.
- Referencia pública correspondiente: `docs/html/es/json/visual.html` y su página EN.
- Índice y README de cada microejemplo. La auditoría Linux distingue el fixture
  histórico 12 del nuevo intercambio 13.
- En el workflow diagnóstico solo se actualizan etiqueta y nombre del fixture
  exportado a `format13-linux-*.flxc`. No se cambia el workflow oficial de releases.

## Pruebas y resultados

Validación ejecutada en Windows x64, MSVC, configuración Release:

| Comprobación | Resultado |
| --- | --- |
| Configuración CMake y compilación limpia, CLI, flx-core y consumidores | Correctas |
| CTest completo | 34/34 aprobadas, 0 fallidas, 0 omitidas |
| Nueva suite `flx-visual-bitmap-tests` | 13 grupos de casos aprobados, incluidos múltiples casos negativos |
| Dibujo real | Orientación, alpha, no tintado, orden, caché y escala anisótropa aprobados |
| Siete microejemplos desde fuente y formato 13 | Tres frames por ejecución y renders idénticos píxel a píxel |
| Máximo 255x255, serialización, truncado y longitudes inválidas | Aprobados |
| Lectura y ejecución del fixture Windows 12 | Aprobadas; fixture intacto |
| Todos los manifiestos de examples y microexamples | 31/31 válidos |
| Schema JSON | Sintaxis, referencias internas y regex de colores comprobadas |
| Workflow diagnóstico | actionlint correcto |
| CLI `flx --version` | 0.4.0 |

La regresión de herencia desde `/archivo:miembro` falló antes de la corrección
y pasó después. La batería se repitió sobre la corrección final.

La validación estricta detectó `cyan` en el microejemplo existente de multicollider:
no era nativo y antes acababa usando el fallback. Se sustituye por `#00FFFF`, sin
añadir un nombre nativo nuevo ni cambiar el sistema de colisión.

Resultados locales guardados en `out/bitmap-ctest.xml`, `out/bitmap-ctest.log` y
`out/bitmap-example-validation.json`. Las capturas reales revisadas visualmente
se conservan en `out/bitmap-previews/`; no son assets de ejecución.

## Microejemplos y comandos

Ejecutar desde la raíz del repositorio, con `flx` en PATH (o usar el ejecutable
local `out/build/v040-audit-release/flx.exe`):

```sh
flx run microexamples/drawing/bitmap/pixel-art/pixel-art.flx
flx run microexamples/drawing/bitmap/palette-composition/palette-composition.flx
flx run microexamples/drawing/bitmap/bitmap-patterns/bitmap-patterns.flx
flx run microexamples/drawing/bitmap/mixed-representations/mixed-representations.flx
flx run microexamples/drawing/bitmap/machine-colors/machine-colors.flx
flx run microexamples/drawing/bitmap/machine-colors/machine-colors-opaque.flx
flx run microexamples/drawing/bitmap/machine-colors/machine-colors-restricted.flx
```

Son, respectivamente, un cohete pixel art; tres tótems con paletas compartidas y
overrides; un tejido geométrico repetido; emblemas con cuatro familias y rotación;
y tres variantes Machine del mismo escenario de fantasmas sobre damero.
Cada carpeta incluye sus manifiestos, JSON, referencias/Machines necesarias y README.
No requieren input ni audio; cerrar la ventana para terminar.

## Límites y ampliaciones

Los aliases son locales, estáticos y se consumen al compilar; no se añaden aliases
a las APIs de scripting. Las referencias de composición seleccionan mapas de
entradas, no composiciones de paletas recursivas. Las cadenas compactas cuentan
valores Unicode escalares, sin normalización de grafemas.

Se mantienen #RGB y #RRGGBB; no se añaden nuevos formatos de color. No hay
paletas dinámicas, animación diferencial, importación de imágenes, matrices
importadas ni API pública de assets. Un reader anterior no puede leer formato 13.
Posibles optimizaciones futuras, tras medir, incluyen atlas internos o deduplicar
texturas por contenido, sin implicar nuevos contratos públicos.

No se ha ejecutado esta implementación nueva en Linux. Tras revisar/publicar,
deben repetirse GCC/Clang, CTest headless y Xvfb y el intercambio binario. Las
pruebas Windows y las capturas no sustituyen esa validación real ni pruebas
manuales de otros equipos, drivers, audio o input físico.
