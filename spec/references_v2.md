# References v2

> Estado: Especificación normativa consolidada (v0.3.0)

## 1. Propósito

Define cómo el lenguaje fuente de FLX expresa y resuelve referencias.

Las referencias existen únicamente durante la compilación.

## 2. Filosofía

```text
Lenguaje fuente
      │
      ▼
Reference Resolution
      │
      ▼
CompiledProject
      │
      ▼
Runtime
```

El Runtime nunca interpreta referencias.

## 3. Espacios

El manifiesto define tres espacios independientes:

- World (`path`)
- Machine
- Input Mapping

Cada uno posee su propia raíz.

## 4. Tipos

- Inline
- Referencia relativa
- Referencia absoluta lógica (`/`)
- Referencia interna (`:`)
- `like`

Todas producen la misma representación compilada.

## 5. Procedencia

Toda referencia conserva el archivo donde fue escrita.

La procedencia nunca cambia por reutilización.

## 6. Sobrescritura

Una referencia sobrescrita pasa a pertenecer al consumidor.

## 7. Like

`like`:

- hereda;
- conserva procedencia;
- detecta ciclos;
- desaparece tras la compilación.

## 8. Scripts

Los scripts son recursos.

Antes del Runtime deben:

- localizarse;
- leerse;
- registrarse;
- convertirse en ResourceId.

## 9. ResourceId

Toda referencia válida termina en un ResourceId.

```text
Referencia
    │
    ▼
ObjectDefinition
    │
    ▼
ResourceRegistry
```

## 10. CompiledProject

El CompiledProject no contiene:

- rutas;
- like;
- referencias;
- miembros internos pendientes.

Solo contiene relaciones mediante ResourceId.

## 11. Validación

validate y compile recorren exactamente el mismo grafo fuente.

Detectan:

- referencias rotas;
- miembros inexistentes;
- scripts inexistentes;
- ciclos;
- colisiones;
- recursos no resolubles.

## 12. Diagnósticos

Los errores utilizan los códigos documentados en `diagnostic-codes.md`.

El diagnóstico debe indicar:

- referencia;
- archivo;
- campo;
- ruta efectiva utilizada.

## 13. Responsabilidades

ProjectManifest define espacios.

JsonLoader interpreta referencias.

ProjectCompiler resuelve referencias.

CompiledProject conserva únicamente ResourceId.

Runtime consume únicamente ResourceId.

## 14. Tests mínimos

- inline;
- relativa;
- absoluta;
- miembro interno;
- like;
- cadena like;
- ciclo;
- script;
- colisión;
- compilaciones consecutivas;
- diagnóstico con ruta efectiva.

## 15. Reglas

REF-001 Toda referencia desaparece antes del Runtime.

REF-002 Toda referencia válida termina en un ResourceId.

REF-003 path define la raíz lógica del mundo.

REF-004 Las referencias relativas se resuelven desde su archivo de origen.

REF-005 Las absolutas se resuelven desde path.

REF-006 La procedencia nunca se pierde.

REF-007 validate y compile recorren el mismo grafo.

## 16. Principio

Las referencias pertenecen al lenguaje del desarrollador.

Los ResourceId pertenecen al lenguaje del Runtime.

La compilación transforma uno en otro.
