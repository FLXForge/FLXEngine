# Project Manifest (`project.flx`)

## Estado

- Documento: especificación normativa
- Ámbito: manifiesto del proyecto FLX
- Versión prevista: 0.3.0
- Estado: consolidación

---

# 1. Propósito

`project.flx` es el punto de entrada de un proyecto FLX.

No describe el mundo del juego; describe cómo localizarlo, con qué Machine debe compilarse y qué metadatos pertenecen al proyecto.

El Runtime nunca debe leer `project.flx`.

---

# 2. Naturaleza de los campos

## Metadata del desarrollador

Campos:

- name
- version
- notes

FLX los conserva, pero no interpreta su significado.

## Configuración del proyecto

Campos:

- title
- engine
- path
- root
- machine
- input.mapping

Estos sí forman parte del contrato del Compiler.

## Configuración de ejecución

No pertenece al proyecto.

Ejemplos:

- window.mode
- debug.*
- fullscreen
- consola

Pertenece al CLI, al host o a la configuración del producto final.

---

# 3. Campos

## name

Nombre humano del proyecto.

- Texto libre.
- No identifica recursos.
- Puede utilizarse como valor por defecto de `title`.

## version

Versión elegida por el desarrollador.

Es texto libre.

FLX no exige SemVer.

No interviene en la compilación ni en la compatibilidad.

## notes

Comentarios del proyecto.

No afectan al comportamiento del motor.

## title

Título visible del juego.

Si no existe puede utilizarse `name`.

## engine

Versión o requisito técnico del motor.

Pertenece a FLX y no a la metadata del desarrollador.

## path

Define la raíz lógica del mundo.

Se resuelve desde el directorio donde se encuentra el `.flx`.

Puede ser relativa, absoluta, `.` o exterior al proyecto.

## root

Identifica el recurso raíz del mundo.

Se resuelve respecto a `path`.

## machine

Selecciona la Machine.

Se resuelve respecto al directorio del `.flx`.

No forma parte del mundo definido por `path`.

## input.mapping

Selecciona el mapping de entrada.

Se resuelve respecto al directorio del `.flx`.

El Runtime nunca vuelve a abrir el archivo fuente.

---

# 4. Campos eliminados

Estos campos dejan de pertenecer al manifiesto:

- screen.width → Machine
- screen.height → Machine
- screen.scale → salida predeterminada de Machine; override temporal mediante `flx run --scale`
- window.mode → CLI mediante `--window-mode`
- debug.* → CLI mediante `--debug-logs`, `--debug-console` y `--debug-collisions`

---

# 5. Resolución de rutas

| Campo | Base |
|------|------|
| path | directorio del `.flx` |
| machine | directorio del `.flx` |
| input.mapping | directorio del `.flx` |
| root | raíz definida por `path` |

---

# 6. CompiledProject

Tras la compilación desaparecen las rutas físicas.

El Runtime recibe únicamente:

- Machine efectiva
- ResourceRegistry
- recurso raíz compilado
- input mapping compilado
- metadata necesaria

---

# 7. Reglas normativas

PM-001 El `.flx` es el punto de entrada del proyecto.

PM-002 El Runtime nunca lee `project.flx`.

PM-003 `path` define la raíz lógica del mundo.

PM-004 `root` se resuelve respecto a `path`.

PM-005 `machine` e `input.mapping` se resuelven respecto al directorio del manifiesto.

PM-006 La metadata del desarrollador nunca modifica el comportamiento de FLX.

PM-007 Las opciones de depuración no pertenecen al proyecto.

PM-008 La configuración de ventana pertenece al entorno de ejecución.

PM-009 La Machine define las capacidades del juego y los valores predeterminados de salida.

PM-010 El Compiler transforma el manifiesto en un `CompiledProject`; el Runtime trabaja únicamente con el resultado compilado.
