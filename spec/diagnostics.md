# Diagnósticos

## Estado

- Documento: especificación normativa
- Ámbito: diagnósticos estructurados de FLX y uso desde aplicaciones anfitrionas

## 1. Representación

`Diagnostics` DEBE conservar en orden las entradas emitidas. Cada
`Diagnostic` separa severidad, código, identificador simbólico, mensaje,
archivo u origen lógico, campo y rango opcional. `hasErrors()` comprueba
la severidad efectiva `Error`. `diagnosticCodeText()` devuelve el código
público estable, mientras que `diagnosticIdentifier()` devuelve su nombre.
El registro de `engine/diagnostics/Diagnostics.cpp` define el catálogo canónico.

## 2. Límites de la operación

`CompiledProjectReader::read()` informa fallos de acceso, formato y validación
en su propio resultado. El lector DEBE conservar los códigos de decodificación
y validación. Una excepción C++ inesperada durante la lectura usa
`FLX-BINARY-00003`. La lectura y la carga de sesión son operaciones distintas.

`FlxSessionResult` acumula los avisos de carga y los errores posteriores de
sesión. `unload()` conserva ese resultado. Una nueva carga admitida lo
reinicia. Un rechazo por estado añade un aviso y conserva el resultado de la
ejecución vigente. Los hosts DEBEN comprobar el retorno de `load()` además del
resultado, ya que un rechazo no cambia el éxito de un proyecto anterior.
Véase [Runtime Session](runtime-session.md#rs-024--resultado-y-diagnósticos).

Los diagnósticos que genera la sesión usan el origen lógico `session` y un
campo de etapa, como `video`, `audio`, `input`, `scripting` o `world`. Los
diagnósticos propagados conservan el origen y campo originales. Los hosts
PUEDEN adaptar la descripción que muestran al usuario y reservar detalles
sensibles para herramientas de diagnóstico locales.

## 3. Códigos para la integración

| Código | Severidad | Condición |
| --- | --- | --- |
| `FLX-BINARY-00003` | Error | Excepción C++ inesperada leyendo o validando el proyecto compilado. |
| `FLX-BINARY-00010` | Error | El archivo compilado no se puede abrir. |
| `FLX-RUNTIME-00003` | Error | Excepción C++ inesperada en la sesión. El campo identifica la etapa. |
| `FLX-RUNTIME-00011` | Error | Configuración de vídeo inválida, incluidas dimensiones lógicas no positivas. |
| `FLX-RUNTIME-00012` | Error | Falta el contexto gráfico requerido. |
| `FLX-RUNTIME-00013` | Error | No se pudo crear el render target. |
| `FLX-RUNTIME-00014` | Error | Fallo al cargar el mundo. La sesión usa este código como respaldo si el mundo no proporcionó un error. |
| `FLX-RUNTIME-00015` | Error | El mundo excedió el límite de spawns durante la carga. |
| `FLX-RUNTIME-00016` | Warning | Se solicitó audio y no hay dispositivo disponible. El proyecto puede funcionar en silencio. |
| `FLX-RUNTIME-00017` | Warning | Se rechazó una nueva carga porque la sesión no estaba en `Empty`. |
| `FLX-RUNTIME-00018` | Error | La configuración de sesión no es válida, por ejemplo una ruta de guardado vacía o con NUL. |
| `FLX-RUNTIME-00019` | Error | El delta de actualización es negativo o no finito. |
