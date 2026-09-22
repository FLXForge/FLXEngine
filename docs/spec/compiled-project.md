# CompiledProject

## Estado

- **Documento:** especificación normativa
- **Ámbito:** representación compilada de un proyecto FLX
- **Versión inicial:** 0.3.0
- **Formato persistente vigente:** `.flxc` v2
- **Estado:** consolidación

---

## 1. Propósito

`CompiledProject` es la representación autónoma, validada y completamente resuelta de un proyecto FLX.

Constituye la frontera entre autoría/compilación y ejecución.

El Compiler produce un `CompiledProject`.

El Runtime consume un `CompiledProject`.

El Runtime no debe conocer cómo se escribió el proyecto fuente.

---

## 2. Flujo

```mermaid
flowchart LR
    Sources[Proyecto fuente] --> Compiler[ProjectCompiler]
    Compiler --> Project[CompiledProject]
    Project --> Runtime[Runtime]
    Project --> Writer[CompiledProjectWriter]
    Writer --> Binary[Archivo .flxc]
    Binary --> Reader[CompiledProjectReader]
    Reader --> Project2[CompiledProject]
    Project2 --> Runtime
```

Los dos caminos de ejecución deben converger en la misma representación:

```text
project.flx → Compiler → CompiledProject → Runtime

.flxc → Reader → CompiledProject → Runtime
```

---

## 3. Contrato central

Un `CompiledProject` válido debe ser suficiente para ejecutar el juego sin volver a consultar:

- `project.flx`;
- JSON;
- YAML;
- JavaScript externo;
- input mapping externo;
- rutas físicas del proyecto fuente.

La ausencia de las fuentes originales no debe impedir su ejecución.

---

## 4. Estructura conceptual

```text
CompiledProject
├── context
├── resources
└── rootId
```

### `context`

Contiene:

- metadata del desarrollador;
- título;
- requisito técnico de FLX;
- Machine efectiva;
- input mapping compilado.

### `resources`

Contiene el `ResourceRegistry`.

Es la fuente única de definiciones compiladas y scripts embebidos.

### `rootId`

Identifica el recurso raíz dentro de `ResourceRegistry`.

La raíz no se conserva como una definición independiente.

---

## 5. Contexto compilado

El contexto compilado contiene únicamente información útil después de la compilación:

```text
Compiled context
├── name
├── version
├── notes
├── title
├── engineRequirement
├── Machine efectiva
├── inputMappingSourceName
├── inputMappingContent
└── inputMapping compilado
```

`name`, `version` y `notes` se conservan como metadata libre.

`title` pertenece a la presentación.

`engineRequirement` no debe confundirse con:

- la versión productora del `.flxc`;
- la versión del formato;
- la versión libre del desarrollador.

La Machine almacenada es efectiva, no una ruta YAML.

El input mapping queda validado y normalizado para ejecución. El contenido
original puede conservarse como procedencia, pero Runtime no debe depender de
leer ni reinterpretar el archivo `.input` original.

---

## 6. ResourceRegistry

`ResourceRegistry` es la fuente única de recursos compilados.

Contiene:

- definiciones de objetos;
- scripts embebidos.

```text
ResourceId
→ ObjectDefinition compilada

ResourceId
→ ScriptResource
```

El Runtime accede a los recursos únicamente mediante `ResourceId`.

---

## 7. Recurso raíz

`rootId` identifica el punto de entrada del mundo.

```text
rootId
→ ResourceRegistry
→ definición raíz
```

La raíz utiliza el mismo mecanismo que hijos, grid, `spawn` y cualquier otro objeto instanciable.

No existe una copia separada `rootDefinition`.

---

## 8. Relaciones entre objetos

Una relación compilada entre un objeto y sus hijos se expresa mediante:

```text
nombre lógico del hijo
→ ResourceId
```

Representación conceptual:

```text
childResources
```

Los objetos almacenados en `ResourceRegistry` no deben conservar árboles completos de hijos embebidos.

La estructura fuente puede utilizar objetos anidados durante la carga y compilación, pero esa forma desaparece antes de registrar el objeto compilado.

---

## 9. Scripts

Los scripts declarados deben quedar:

- localizados;
- leídos;
- identificados;
- embebidos;
- registrados.

Cada objeto compilado conserva únicamente las identidades de sus scripts resueltos.

```text
resolvedScriptPaths
→ ResourceRegistry
→ ScriptResource
```

El Runtime no abre archivos JavaScript externos.

---

## 10. Información que no pertenece a CompiledProject

Un `CompiledProject` no debe contener:

- rutas físicas necesarias para ejecutar;
- `project.flx`;
- referencias JSON pendientes;
- `like`;
- miembros internos pendientes;
- rutas de scripts pendientes;
- rutas YAML;
- opciones de depuración;
- modo de ventana;
- override temporal de escala;
- opciones del CLI;
- estado vivo del Runtime;
- `RuntimeObject`;
- definiciones raíz duplicadas;
- árboles completos duplicados.

---

## 11. Invariantes

### CP-001 — Identidad raíz

`rootId` debe ser una cadena no vacía.

### CP-002 — Recurso raíz existente

`rootId` debe existir como objeto dentro de `ResourceRegistry`.

### CP-003 — Fuente única

`ResourceRegistry` es la única fuente de definiciones compiladas.

### CP-004 — Hijos válidos

Todo `childResources` debe apuntar a un objeto existente.

### CP-005 — Scripts válidos

Todo script resuelto debe existir en el registro de scripts.

### CP-006 — Sin hijos embebidos

Un objeto compilado registrado no debe conservar definiciones hijas embebidas.

### CP-007 — Sin referencias fuente

No deben existir referencias, rutas o herencias pendientes.

### CP-008 — Machine efectiva

La Machine almacenada debe ser utilizable directamente por Runtime.

### CP-009 — Input mapping autónomo

El input mapping no debe requerir acceso a su archivo original.

### CP-010 — Sin configuración del host

Las opciones de ejecución no forman parte del proyecto compilado.

### CP-011 — Resultado fallido

Cuando una compilación falla, `CompilationResult.project` no se considera consumible.

### CP-012 — Equivalencia de ejecución

`run` y `run-compiled` deben entregar al Runtime proyectos semánticamente equivalentes.

---

## 12. Validación

Las invariantes deben validarse:

- al finalizar la compilación;
- antes de escribir un `.flxc`;
- después de leer un `.flxc`;
- antes de cargar el proyecto en Runtime.

La validación defensiva del Runtime no sustituye a la del Compiler o del lector binario.

---

## 13. Persistencia `.flxc`

`.flxc` es la representación persistente de un `CompiledProject`.

Formato vigente:

```text
FormatVersion = 2
```

Incluye conceptualmente:

```text
magic
formatVersion
producerVersion
compiled context
rootId
object resources
script resources
```

No incluye:

- `rootDefinition` independiente;
- árboles `children` embebidos;
- rutas fuente necesarias para ejecutar.

---

## 14. Versiones diferenciadas

FLX distingue:

- versión del formato `.flxc`;
- versión productora;
- `engineRequirement`;
- versión libre del desarrollador.

No deben mezclarse ni sobrescribirse entre sí.

---

## 15. Compatibilidad del formato

La versión 0.3.0 no garantiza compatibilidad con formatos `.flxc` anteriores.

Un lector debe rechazar explícitamente una versión no soportada.

Antes de estabilizar `.flxc` como contrato público deberán especificarse:

- tipos binarios;
- tamaños;
- endianness;
- representación de booleanos;
- representación de números;
- límites máximos;
- política de compatibilidad.

---

## 16. Determinismo

La serialización debe producir un orden estable para los registros.

Esto facilita:

- tests;
- comparación de binarios;
- reproducibilidad;
- diagnóstico;
- futuras verificaciones de integridad.

---

## 17. Responsabilidades

### ProjectCompiler

- transforma fuentes;
- asigna identidades;
- resuelve relaciones;
- embebe scripts;
- construye contexto;
- valida invariantes.

### ResourceRegistry

- conserva recursos compilados;
- impide duplicados silenciosos;
- permite búsqueda por `ResourceId`.

### CompiledProjectWriter

- valida;
- serializa;
- no redefine el proyecto.

### CompiledProjectReader

- valida cabecera y formato;
- reconstruye el proyecto;
- rechaza duplicados;
- valida invariantes.

### Runtime

- consume el proyecto;
- obtiene la raíz desde `rootId`;
- instancia objetos;
- nunca reinterpreta fuentes.

---

## 18. Fuente y compilado

La forma fuente y la forma compilada representan fases distintas.

El contrato exige que todo objeto registrado esté normalizado:

```text
children embebidos = vacío
childResources = relaciones efectivas
scripts externos = resueltos
sourcePath = identidad portable o información diagnóstica
```

Antes de la v1 debe evaluarse expresamente si fuente y compilado requieren tipos estructuralmente distintos.

No puede mantenerse como deuda conocida una representación híbrida o provisional.

---

## 19. Tests mínimos

Deben existir pruebas para:

- raíz válida;
- `rootId` vacío;
- raíz inexistente;
- hijo inexistente;
- script inexistente;
- objeto con `children` embebidos;
- raíz obtenida desde `ResourceRegistry`;
- `spawn`;
- hijos automáticos;
- hijos manuales;
- grid;
- ejecución sin fuentes;
- escritura y lectura `.flxc`;
- rechazo de formato anterior;
- recursos duplicados;
- equivalencia entre fuentes y binario;
- orden determinista.

---

## 20. Principio final

`CompiledProject` no es una copia del proyecto fuente.

Es el resultado de haberlo comprendido, resuelto, normalizado y preparado para ejecución.

```text
fuente
→ significado
→ CompiledProject
→ ejecución
```

El Runtime recibe significado, no documentos.
