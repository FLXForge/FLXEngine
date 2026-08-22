# Engine

## Estado

- Documento: especificación normativa
- Ámbito: host de ejecución
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Propósito

`Engine` es el host de ejecución de FLX. Recibe un `CompiledProject`, crea el entorno físico de ejecución, conecta los subsistemas y ejecuta el ciclo principal. No interpreta archivos fuente ni recompila proyectos.

## 2. Flujo

```text
CompiledProject + RunOptions
        ↓
      Engine
        ↓
Video / Audio / Input
        ↓
 ScriptEngine
        ↓
 RuntimeWorld
        ↓
  Main Loop
        ↓
 EngineResult
```

## 3. Responsabilidades

Engine debe:
- recibir `CompiledProject` y `RunOptions`;
- validar la configuración del host;
- inicializar vídeo, audio e input;
- conectar `ScriptEngine` con `RuntimeWorld`;
- ejecutar el bucle principal;
- finalizar limpiamente los recursos;
- devolver un `EngineResult`.

No debe:
- leer `.flx`;
- cargar JSON, YAML o JavaScript;
- resolver referencias;
- recompilar proyectos.

## 4. API

Única entrada pública:

```text
Engine::run(CompiledProject, RunOptions)
```

## 5. RunOptions

Contiene únicamente opciones temporales de ejecución:
- límite de frames;
- modo de ventana;
- sobrescritura de escala;
- opciones de depuración.

No modifica el proyecto compilado.

## 6. Inicialización

```text
configuración
→ validación de vídeo
→ ventana
→ render target
→ audio
→ input
→ ScriptEngine
→ RuntimeWorld
```

Si una fase falla, no se entra en el bucle principal.

## 7. RuntimeWorld

La carga devuelve un resultado explícito. Si falla:
- se conservan los diagnósticos;
- Engine devuelve `RuntimeWorldLoadFailed`;
- no se ejecuta ningún frame.

## 8. EngineResult

Siempre contiene:

```text
success
exitReason
diagnostics
framesExecuted
```

Motivos:
- WindowClosed
- ScriptRequestedExit
- FrameLimitReached
- InitializationFailed
- RuntimeLoadFailed

## 9. Validaciones

Antes de crear la ventana:
- dimensiones positivas;
- escala positiva;
- ausencia de overflow.

Después:
- ventana válida;
- render target válido;
- Runtime cargado.

## 10. Shutdown

Es idempotente. Solo libera recursos realmente inicializados y funciona también tras fallos parciales.

## 11. Política

Una instancia de Engine admite una única llamada a `run()`.

Una segunda llamada produce:

```text
FLX-RUNTIME-00010 EngineAlreadyRun
```

## 12. Diagnostics

- FLX-RUNTIME-00010 EngineAlreadyRun
- FLX-RUNTIME-00011 InvalidVideoOutputConfiguration
- FLX-RUNTIME-00012 WindowInitializationFailed
- FLX-RUNTIME-00013 RenderTargetInitializationFailed
- FLX-RUNTIME-00014 RuntimeWorldLoadFailed

## 13. Relación con Machine

Machine define las capacidades lógicas.

Engine decide cómo alojarlas físicamente.

## 14. Tests mínimos

- carga correcta;
- fallo de Runtime;
- límite de frames;
- salida solicitada por script;
- segunda ejecución;
- vídeo inválido;
- overflow;
- propagación de diagnósticos;
- propagación al CLI.

## 15. Reglas normativas

- ENG-001 Engine recibe únicamente `CompiledProject` y `RunOptions`.
- ENG-002 Engine devuelve siempre `EngineResult`.
- ENG-003 No entra en el loop si la inicialización falla.
- ENG-004 No entra en el loop si `RuntimeWorld` falla.
- ENG-005 Una instancia admite una sola ejecución.
- ENG-006 El shutdown es seguro tras inicialización parcial.
- ENG-007 El CLI obtiene el resultado desde `EngineResult`.
- ENG-008 Engine no accede a archivos fuente.

## 16. Principio

```text
CompiledProject
      ↓
    Engine
      ↓
 RuntimeWorld
      ↓
    Juego
```
