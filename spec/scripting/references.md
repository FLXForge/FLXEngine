# References / Context / hierarchy

## Estado
- Documento: especificación normativa
- Ámbito: referencias Runtime y navegación estructural
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Principio

`RuntimeObject` JS es una referencia pública viva, no copia C++, espejo JSON, handle persistente ni estado serializable.

> El scripting conserva identidad; el Runtime conserva el mundo vivo.

El objeto de un hook es punto de partida, no frontera del mundo.

## 2. Vigencia

Wrapper normal válido durante la invocación activa mientras su instancia sigue viva.

`kill()` hace alive=false inmediatamente; desde ahí la referencia normal deja de ser válida para operaciones que requieren un vivo.

Para persistir identidad: guardar `object.id` string y resolver después con `find_id()`.

`dead(object)` es excepción controlada de contexto final.

## 3. API

```ts
find_id(id: string): RuntimeObject | undefined;
find_name(name: string): RuntimeObject[];
find_parent(object: RuntimeObject): RuntimeObject | undefined;
find_children(object: RuntimeObject): RuntimeObject[];
```

Consultan vivos. `dead(object)` puede usarse como contexto estructural final; resultados siguen siendo vivos.

Orden de arrays: entrada al mundo, antiguo→reciente. Son snapshots JS.

## 4. Parent estructural

Se fija al originarse la instancia y es estable en v0.3. No hay reparenting público.

Para child normal:

```text
parent/child ≠ ownership
```

## 5. Component

`component:true` añade pertenencia lógica sobre la estructura, pero no cambia navegación.

```text
tank
└─ turret [component]
   └─ cannon [component]
```

```text
find_parent(cannon) → turret
find_parent(turret) → tank
find_children(tank) → incluye turret vivo
```

`find_children` incluye children normales y components; no colapsa.

Component tiene consecuencias transversales (lifecycle, State, Ray) definidas en `component.md`, sin convertir parent estructural en ownership genérico.

No existe API pública de “logical entity” en v0.3.

## 6. Follow / Carry / Attach

Follow, carry y attach conservan sus contratos propios.

```text
Component ≠ Attach ≠ Carry ≠ Follow
```

Attach no cambia parent ni convierte en component.

## 7. Diagnósticos

- RuntimeObject no persistible en local/global;
- wrapper de otro hook inválido;
- referencia normal a instancia muerta inválida;
- no conversión automática RuntimeObject↔id.

## 8. Invariantes

- REF-001 wrapper no es identidad persistente.
- REF-002 id es identidad pública de instancia.
- REF-003 find normales devuelven vivos.
- REF-004 parent estructural estable.
- REF-005 normal hierarchy no implica ownership.
- REF-006 Component no colapsa navegación estructural.
- REF-007 Component añade relación lógica explícita, no ownership genérico.
- REF-008 Attach/carry/follow/Component son conceptos distintos.
