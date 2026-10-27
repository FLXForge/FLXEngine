# Runtime Session

## Estado

- Documento: especificación normativa
- Ámbito: biblioteca C++ y ejecución integrada del runtime
- Versión inicial: 0.4.0
- Estado: consolidado
- Idioma canónico: español

## 1. Propósito

FLX ejecuta mundos declarativos. El host decide dónde, cuándo y durante cuánto
tiempo se ejecutan. Standalone e integración DEBEN utilizar el mismo runtime,
sin modificar Machine, scripting, lifecycle ni el formato `.flxc`.

Esta versión permite proyectos sucesivos en un proceso con dispositivos
persistentes. No establece ejecución simultánea de sesiones ni cambios de mundo
desde JavaScript.

## 2. Responsabilidades y biblioteca

### RS-001 — Host y sesión

El host DEBE crear y destruir ventana, contexto gráfico y dispositivo de audio.
DEBE controlar el loop, el delta, el frame físico, la presentación y la
configuración global de logging/raylib. La sesión NO DEBE llamar a `InitWindow`,
`CloseWindow`, `InitAudioDevice`, `CloseAudioDevice`, `BeginDrawing` ni `EndDrawing`.

La sesión DEBE poseer la ejecución: proyecto, mundo, contexto JS, input lógico,
fade, superficie lógica y recursos de audio. Cargar o descargar una sesión NO
DEBE reconfigurar el logger ni el callback/nivel de logging de raylib. El proveedor
físico predeterminado de input es un servicio sin estado lógico de sesión.

### RS-002 — Unidad de ejecución

`FlxSession` DEBE representar un proyecto cargado. La API es de un solo hilo y
no reentrante: el host NO DEBE invocar carga, actualización, renderizado,
detención o descarga desde callbacks del proveedor físico ni concurrentemente.
El proveedor NO DEBE destruir la sesión que lo está consultando.

Una carga solo se admite en `Empty`. Cargar en cualquier otro estado DEBE devolver
`false` sin sustituir el proyecto ni alterar su estado o resultado. El host DEBE
usar `unload()` antes de cargar de nuevo.

### RS-003 — Targets CMake

`flx-core` DEBE ser una biblioteca estática C++20 de producción, sin punto de
entrada ni comandos CLI. Sus dependencias y requisitos de inclusión/enlace DEBEN
propagarse al consumidor mediante el target CMake. La cabecera de integración es
`<core/FlxSession.h>`; NO se ofrece `RuntimeWorld` como interfaz de integración.

`flx-cli` agrupa los comandos standalone y enlaza `flx-core`; `flx` contiene el
punto de entrada. `flx-test-engine` es una variante separada de las mismas fuentes
con `FLX_TESTING`, necesaria por los hooks/layouts de pruebas. El consumidor de
producción NO DEBE depender de esa variante. No se garantiza ABI estable ni se
introducen bibliotecas dinámicas, backends o dependencias nuevas.

## 3. Proyecto y propiedad

### RS-004 — Fuente de ejecución

`load()` recibe un `CompiledProject` y DEBE validar sus invariantes conforme a
[CompiledProject](compiled-project.md). La ejecución NO DEBE releer `.flx`, YAML,
JSON, JS ni mapping externo. Compiler y Reader de `.flxc` convergen en el mismo
modelo compilado.

### RS-005 — Propiedad

`load(const CompiledProject&, ...)` DEBE crear una copia propia, incluido el
registro de recursos. El argumento original PUEDE modificarse o destruirse al
retornar. Las referencias del mundo y del scripting DEBEN apuntar al proyecto
propio, que DEBE sobrevivir a todos sus consumidores. La sesión no es copiable
ni movible.

### RS-006 — Compatibilidad binaria

La modularización NO DEBE cambiar el codec, su versión, su validación ni los
contratos declarativos. La versión de la biblioteca y la del formato son conceptos
distintos; esta especificación no introduce una nueva versión de `.flxc`.

## 4. API pública y ciclo de vida

La API C++ de producción es:

```cpp
bool load(const CompiledProject&, const FlxSessionOptions& = {});
bool update(float deltaTime);
bool render();
void requestStop();
void unload();
FlxSessionState state() const;
bool finished() const;
const FlxSessionResult& result() const;
Texture2D output() const;
```

`load()`, `update()` y `render()` DEBEN devolver `true` si completan su operación,
incluso si se solicita `exit()` durante ella. Un `false` PUEDE indicar rechazo por
estado, vídeo deshabilitado en render o un fallo; el host DEBE consultar estado y
diagnósticos para distinguirlos. La API no descarga automáticamente al finalizar.

`FlxSessionOptions` contiene `enableVideo=true`, `enableAudio=true`,
`debugCollisions=false` y `inputProvider=nullptr`. Son opciones del host, no
capacidades nuevas de Machine. Deshabilitar vídeo permite ejecución lógica sin
superficie ni callbacks `draw`; `render()` devuelve `false`. Deshabilitar audio
omite la materialización sonora sin modificar sus definiciones. Estas opciones
permiten pruebas lógicas sin dispositivos y no modifican el modo standalone.

### RS-007 — Estados

| Estado | Significado |
| --- | --- |
| `Empty` | Sin recursos de ejecución |
| `Ready` | Mundo cargado; `born()` y sus spawns ya estabilizados |
| `Running` | Ha comenzado una actualización válida |
| `Finished` | Terminación solicitada desde JS o desde el host |
| `Failed` | Error de carga o ejecución; recursos propios liberados |

`finished()` DEBE ser verdadero en `Finished` y `Failed`, y falso en los demás.
La ausencia de objetos vivos NO constituye por sí sola una condición nueva de fin.

### RS-008 — Transiciones y operaciones admitidas

| Operación / condición | Transición |
| --- | --- |
| carga correcta | `Empty → Ready` |
| carga fallida | `Empty → Failed` |
| primera actualización válida | `Ready → Running` |
| `exit()` en `born()` | `Empty → Ready → Finished` |
| `exit()` en update/draw o parada del host | `Ready/Running → Finished` |
| delta inválido o excepción C++ de ejecución | `Ready/Running → Failed` |
| descarga explícita | cualquier estado `→ Empty` |

`update()` y `render()` NO DEBEN ejecutar callbacks en `Empty`, `Finished` o
`Failed`; devuelven `false`. `render()` PUEDE ejecutarse en `Ready`, sin cambiarlo
a `Running`. Una ejecución finalizada solo puede reiniciarse mediante descarga y
nueva carga. Una operación rechazada por estado no añade un error al resultado.

### RS-009 — Descarga

`unload()` DEBE ser idempotente y seguro tras cargas parciales o fallidas.
DEBE invalidar `output()` y liberar el estado temporal. NO DEBE cerrar dispositivos
ni generar artificialmente `kill()`/`dead()` sobre todos los objetos al desmontar
el mundo. La política de muerte individual permanece en [Lifecycle](scripting/lifecycle.md).

### RS-010 — Orden de destrucción

El contexto y callbacks JS DEBEN destruirse antes del mundo al que referencian;
el mundo DEBE desaparecer antes del proyecto propio. Sonidos/streams y render
target DEBEN liberarse mientras sus dispositivos sigan disponibles.

El host DEBE conservar contexto gráfico, dispositivo de audio y proveedor prestado
hasta terminar `unload()` o destruir la sesión. Cerrar dispositivos antes viola
la precondición; la sesión no recupera un contexto externo destruido.

### RS-011 — Finalización diferida

`exit()` DEBE registrar una solicitud. La sesión DEBE observarla después de que
retorne la construcción del mundo, la actualización o el renderizado que estaba
ejecutando callbacks. NO DEBE destruir el contexto dentro del callback.
El código posterior a `exit()` en ese callback conserva su ejecución normal.
Finalizar detiene los recursos de audio; contexto, mundo y última superficie se
conservan hasta `unload()` o destrucción.

## 5. Tiempo y salida gráfica

### RS-012 — Delta del host

`update()` DEBE recibir un delta finito y no negativo. Un valor negativo o no
finito DEBE producir `Failed`, diagnóstico y `RuntimeFailed`. El delta válido
DEBE transmitirse sin el clamp standalone a input, scripting, mundo y fade.
La sesión NO DEBE consultar `GetFrameTime()` ni tener un loop propio. El audio
físico conserva el reloj del backend para su reproducción y drenaje.

`Engine` DEBE seguir limitando su delta a `1/30` s y manteniendo su objetivo de
60 FPS. No se cambian las fases del frame de [Runtime](runtime.md).

### RS-013 — Renderizado lógico

Con vídeo habilitado, el host DEBE proporcionar un contexto raylib válido antes
de `load()`. La sesión DEBE crear un render target a resolución lógica de Machine,
proyectar su color de fondo y aplicar el filtro indicado por `video.output.smoothing`.
Mundo, primitivas, wrap, orden visual, colores y fade DEBEN conservar su semántica.

`render()` DEBE dibujar únicamente esa superficie usando el ciclo de textura,
sin comenzar o finalizar el frame físico. El host DEBE llamarlo en una frontera
2D normal, fuera de un `BeginTextureMode` o shader personalizado propio; después
PUEDE iniciar su composición en otra superficie o en la ventana. No se ofrece
restauración genérica de estados gráficos externos anidados.

### RS-014 — Acceso a la superficie

`output()` DEBE devolver una copia no propietaria de `Texture2D`. El host NO DEBE
liberarla ni conservarla después de unload/fallo/destrucción. Su identificador
DEBE ser cero sin superficie. El contenido solo se considera producido después
de un `render()` correcto; una carga por sí sola no pinta un frame.

En `Finished`, la última superficie renderizada PUEDE seguir presentándose hasta
unload. Para presentar la textura de framebuffer en orientación convencional,
el host DEBE invertir verticalmente su rectángulo fuente, según la convención de
raylib. Posición, escala y composición final pertenecen al host; la sesión NO
DEBE asumir que ocupa toda su ventana. `video.output.scale` sigue perteneciendo a
la presentación standalone, no a los argumentos de los bindings.

## 6. Audio

### RS-015 — Dispositivo prestado

Con audio habilitado, el host DEBE inicializar el dispositivo antes de `load()`.
Sin él, la carga DEBE fallar con diagnóstico. La sesión utiliza `attachDevice()`;
NO DEBE inicializar ni cerrar el dispositivo. La modalidad propietaria de
`AudioSystem::init()` conserva su comportamiento para consumidores existentes.

### RS-016 — Recursos de ejecución

Sonidos, música, streams, voces y pausa pertenecen al `AudioSystem` de la sesión.
Finalizar o descargar DEBE detener y liberar esas reproducciones sin cerrar el
dispositivo. Una nueva carga NO DEBE heredar voces, buffers o estado musical.
Se conservan síntesis incremental, Machine y políticas de [Audio](audio.md).

## 7. Input

### RS-017 — Proveedor físico y mapping

La sesión DEBE conservar `PhysicalInputProvider`. `inputProvider` es prestado;
`nullptr` selecciona las consultas raylib existentes. El proveedor del host DEBE
mantenerse válido mientras la sesión esté cargada. Mapping, restricciones de Input
Chip y estados pressed/down/released pertenecen a una instancia nueva de `InputSystem`.

### RS-018 — Aislamiento lógico

Una carga nueva NO DEBE heredar botones, direcciones, órdenes ni buffers lógicos
anteriores. Un botón físicamente mantenido PUEDE producir un nuevo pressed al
comenzar una ejecución nueva; eso no equivale a heredar el estado lógico anterior.
El host sigue siendo responsable de obtener/actualizar la entrada física.

## 8. Mundo, scripting y persistencia

### RS-019 — Estado del mundo

Objetos, identificadores, relaciones, mecánicas, timers, colisiones, attachments
y fade DEBEN pertenecer a la ejecución. Los offsets vivos de attachment DEBEN
pertenecer a la instancia, separados de su offset declarativo original. Un
runtimeId reutilizado por otra carga NO DEBE recuperar offsets anteriores.

Los servicios globales legítimos del host (logger, raylib y su generador aleatorio)
NO se convierten por ello en estado de mundo ni se reinicializan al cargar.

### RS-020 — Contexto JS nuevo

Cada carga DEBE crear un runtime/contexto JS nuevo, sin módulos, variables,
referencias temporales ni callbacks del anterior. Las referencias del proyecto
DEBEN seguir válidas hasta destruir ese contexto. Los errores de creación de
QuickJS NO DEBEN dejar recursos parciales propios sin liberar.

### RS-021 — Persistencia explícita

Descargar NO DEBE eliminar datos guardados mediante las capacidades de persistencia
existentes. El contexto temporal nuevo no implica borrar los archivos persistidos.
No se introduce una nueva política de almacenamiento entre aplicaciones.

## 9. Finalización y errores

### RS-022 — Salida desde scripting

`exit()` conserva su API y termina únicamente la sesión en integración. NO DEBE
cerrar el proceso ni la ventana anfitriona. El motivo DEBE ser
`ScriptRequestedExit`, con éxito si no hay error fatal.

### RS-023 — Parada del host

`requestStop()` en `Ready` o `Running` DEBE finalizar con `HostRequestedStop` y
liberar la reproducción sonora. En los demás estados NO DEBE alterar el resultado.
No ejecuta una actualización adicional. El cierre de la ventana es una decisión
del host, no un error nuevo de mundo.

### RS-024 — Resultado y diagnósticos

`result()` DEBE permitir consultar `success`, `exitReason`, `diagnostics` y
`framesExecuted`; `state()` proporciona el estado actual. Los motivos son `None`,
`ScriptRequestedExit`, `HostRequestedStop`, `LoadFailed` y `RuntimeFailed`.
La carga correcta y la terminación solicitada son exitosas. Los fallos DEBEN
conservar los diagnósticos de carga y añadir el diagnóstico de sesión pertinente.

El contador de sesión cuenta actualizaciones completadas sin solicitar `exit()`.
Renderizar varias veces sin actualizar NO incrementa ese contador. `EngineResult`
conserva su contador histórico de frames presentados; un exit antes de draw no
suma un frame. Los resultados de sesión se conservan tras unload hasta la
siguiente carga, que DEBE comenzar con un resultado nuevo.

### RS-025 — Fallo y recuperación

Una excepción C++ estándar durante carga, input, actualización o renderizado DEBE
convertirse en fallo diagnosticado y liberar recursos propios. Un fallo NO DEBE
inutilizar los dispositivos; tras unload se DEBE poder volver a cargar.
No se garantiza recuperación de memoria agotada que impida incluso construir
un diagnóstico, de un proveedor destruido o de un dispositivo invalidado por el host.

Las excepciones JavaScript registradas por los callbacks conservan la política
no fatal de [Runtime, sección 16](runtime.md#16-errores-de-scripting). NO se convierten
silenciosamente en fallos fatales de sesión ni cambian el contrato de scripting.

## 10. Compatibilidad standalone

### RS-026 — Host Engine

`Engine` DEBE delegar ejecución lógica en `FlxSession`, conservando una sola
llamada a `run()` por instancia, opciones, dimensiones, fullscreen, letterboxing,
escala, clamp de delta, diagnósticos y códigos CLI. DEBE descargar la sesión antes
de cerrar audio y ventana. Su configuración global de logging sigue siendo propia
del host standalone, no una responsabilidad de sesión.

### RS-027 — Equivalencia

`flx run` y `flx run-compiled` DEBEN converger en la misma sesión. Una Machine y
proyecto equivalentes DEBEN conservar su comportamiento, salvo el tiempo y la
presentación que controla cada host. Compilación/validación y formato no cambian.

## 11. Invariantes y validación

### RS-028 — Regresión y biblioteca

Las suites existentes DEBEN conservar sus resultados. Un consumidor independiente
DEBE poder compilar/enlazar `flx-core` y ejecutar sesiones sin CLI ni `FLX_TESTING`.
Las listas de fuentes y configuración común DEBEN compartirse con la variante de
pruebas sin contaminar producción con hooks.

### RS-029 — Ciclo de vida

`flx-session-tests` DEBE cubrir carga, descarga repetida, carga rechazada, propiedad
del proyecto, fallo, recuperación, salida desde born/action, parada del host,
delta e input físico fallidos y la política no fatal de excepciones JS.

### RS-030 — Aislamiento

Las pruebas DEBEN comprobar nuevo contexto JS, objetos/identidades, attachments,
input y ejecución desde binario sin fuentes. `flx-attachment-isolation-tests`
DEBE detectar que cargar otro mundo no borra el offset de una instancia existente.
`flx-audio-stream-tests` DEBE comprobar liberación de recursos con dispositivo
prestado sin cerrarlo.

### RS-031 — Host persistente y dispositivos

`flx-session-device-tests` DEBE ejecutar dos proyectos sucesivos dentro del mismo
contexto gráfico y dispositivo de audio, con update, renderizado, composición y
presentación efectivos. DEBE comprobar colores/superficie, reproducción, fin,
fallo parcial, nueva sesión y destrucción conservando los dispositivos. Requiere
ventana/contexto y audio reales; devuelve 77 si se solicita headless o no están
disponibles. Ese caso es omitido, no superado.

### RS-032 — CLI y equivalencia

Se DEBEN conservar las suites de CLI, Engine, compilación y roundtrip `.flxc`.
La prueba de dispositivos DEBE comparar la finalización y el recuento de frames
con `Engine` standalone. No basta compilar un consumidor para afirmar que se ha
validado un host persistente real.

Comandos de validación (desde el build correspondiente):

```text
ctest --output-on-failure
ctest --output-on-failure -R "flx-session|flx-core-consumer|flx-attachment"
```

`FLX_TEST_HEADLESS=1` permite ejecutar la parte lógica sin dispositivos. Las suites
de gráficos y de host persistente se DEBEN ejecutar además sin esa variable para
validarlas. Las omisiones y fallos DEBEN declararse expresamente en el informe.

## 12. Ejemplo de integración

En CMake, el ejecutable del host enlaza únicamente la biblioteca de producción:

```cmake
add_subdirectory(path/to/FLXEngine flx-build EXCLUDE_FROM_ALL)
add_executable(my-host main.cpp)
target_link_libraries(my-host PRIVATE flx-core)
```

Con `project` ya compilado, el esquema mínimo C++20 es:

```cpp
#include <core/FlxSession.h>

int runProject(const CompiledProject& project) {
    InitWindow(960, 640, "Host");
    if (!IsWindowReady()) return 1;
    InitAudioDevice();
    if (!IsAudioDeviceReady()) { CloseWindow(); return 1; }
    int status = 1;
    {
        FlxSession session;
        if (session.load(project)) {
            while (!WindowShouldClose() && !session.finished()) {
                if (!session.update(GetFrameTime())) break;
                if (session.finished()) break;
                if (!session.render()) break;
                const auto texture = session.output();
                BeginDrawing();
                ClearBackground(BLACK);
                DrawTexturePro(texture,
                    {0, 0, float(texture.width), -float(texture.height)},
                    {20, 20, float(texture.width), float(texture.height)},
                    {0, 0}, 0, WHITE);
                EndDrawing();
            }
            session.requestStop();
            status = session.result().success ? 0 : 1;
        }
        session.unload();
    }
    CloseAudioDevice();
    CloseWindow();
    return status;
}
```

Para otro proyecto, el host PUEDE conservar los dispositivos, usar unload/load
sobre la misma instancia o crear otra sesión, y repetir el ciclo. El test de
consumidor mínimo y el test de dispositivos verifican responsabilidades distintas.

## 13. Fuera de alcance y cierre

No se implementan sesiones simultáneas, escenas/campañas, cambio automático de
mundo, nuevas APIs JS, backends, formatos, ABI estable, plugins ni integración
específica con 80's Like. La validación de dos proyectos consecutivos en un host
persistente NO constituye una garantía de ejecución simultánea.

El cierre exige biblioteca enlazable, API de sesión operativa, propiedad de
recursos/dispositivos verificada, aislamiento/recuperación y compatibilidad CLI.
La demostración o integración de una aplicación externa concreta sigue siendo
independiente de esta capacidad general.
