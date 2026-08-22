# Diagnostics v2

> Estado: Especificación normativa consolidada (v0.3.0)

## 1. Propósito

El sistema de Diagnostics constituye el contrato transversal de comunicación de incidencias de FLX.
Su objetivo es proporcionar una representación estable, legible por personas y herramientas, independiente del componente que produzca el diagnóstico.

## 2. Arquitectura

```text
Producer
    │
    ▼
Diagnostic
    │
    ▼
Diagnostics
    │
    ▼
OperationResult
    │
    ▼
Presentation (CLI / Tool / IDE)
```

Los productores crean diagnósticos.
La presentación únicamente los transforma.

## 3. Modelo

```text
Diagnostic
├── severity
├── code
├── identifier
├── file
├── field
├── message
└── range (opcional)
```

- severity: info | warning | error
- code: formato estable `FLX-DOMAIN-NNNNN`
- identifier: nombre estable del problema.
- file: cadena. Nunca null.
- field: cadena. Nunca null.
- message: explicación humana.
- range: localización opcional.

## 4. DiagnosticCode

El código identifica el problema.

El mensaje puede evolucionar.

El identifier mantiene un nombre estable.

Los códigos públicos se documentan exclusivamente en `diagnostic-codes.md`.

## 5. Diagnostics

Una colección conserva:

- orden de producción;
- duplicados;
- severidad;
- contexto.

Nunca imprime ni serializa.

## 6. OperationResult

```text
success
diagnostics
value (opcional)
```

Existe una única regla:

`success == false` ⇔ existe al menos un diagnóstico de severidad error.

## 7. ExitCode

ExitCode representa el resultado global de una operación.

Diagnostic representa incidencias concretas.

Nunca sustituyen uno al otro.

## 8. Presentación

Formato texto:

```text
error FLX-RESOURCE-00010 MissingReferencedResource
```

Formato JSON:

```json
{
  "severity":"error",
  "code":"FLX-RESOURCE-00010",
  "identifier":"MissingReferencedResource",
  "file":"",
  "field":"",
  "message":"..."
}
```

`range` solo aparece cuando existe.

## 9. Compatibilidad

Una vez publicado un código:

- no cambia de significado;
- no se reutiliza;
- mantiene identifier;
- puede cambiar únicamente el mensaje.

## 10. Dependencias

Todos los subsistemas pueden producir Diagnostics.

Diagnostics no depende de:

- CLI
- Compiler
- Runtime
- Builder
- Tools

## 11. Tests mínimos

- creación;
- append;
- orden;
- hasErrors;
- serialización JSON;
- salida texto;
- UTF-8;
- estabilidad de code e identifier.

## 12. Reglas

DIAG-001 Todo Diagnostic tiene severity.

DIAG-002 Todo Diagnostic tiene code.

DIAG-003 Todo Diagnostic tiene identifier.

DIAG-004 file y field existen siempre como cadenas.

DIAG-005 range es opcional.

DIAG-006 Solo error provoca failure.

DIAG-007 Diagnostics nunca imprime.

DIAG-008 Diagnostics nunca serializa.

## 13. Pendiente

- details estructurado.
- localización de mensajes.

## 14. Principio

La identidad del problema pertenece a `code` e `identifier`.

La explicación pertenece al mensaje.
