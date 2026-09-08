# Lifecycle

## Estado
- Documento: especificación normativa
- Ámbito: lifecycle y participación en el mundo
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Modelo

```text
entra en mundo
→ born() [una vez]
→ ALIVE
→ kill()
→ alive = false INMEDIATAMENTE
→ dead() [una vez]
→ cleanup
→ sale del mundo
```

Muerte lógica inmediata no significa eliminación física inmediata.

## 2. Spawn

`spawn(object, childName)` solicita creación a partir de un child declarado.

Fronteras Runtime relevantes:
- desde action: se integra antes de motion;
- desde motion: se integra después de motion;
- desde collision: se integra al terminar Collision y no participa en esa misma fase;
- desde born: se estabiliza durante construcción inicial.

El request sobrevive aunque el source muera antes de integrarse.

## 3. Children y parent estructural

La jerarquía declara contexto de creación y parent estructural.

Por defecto:

```text
hierarchy ≠ ownership
```

Un child normal mantiene lifecycle independiente:
- kill(parent) no mata automáticamente al child;
- kill(child) no afecta al parent.

## 4. Component y lifetime

Un child con:

```json
"component": true
```

es parte lógica de su parent.

Mantiene lifecycle propio mientras parent vive, pero el lifetime de la entidad establece una cascada de muerte.

```text
tank
└─ turret [component]
   └─ cannon [component]
```

`kill(tank)` mata tank, turret y cannon mediante el mismo lifecycle normal.

Un child normal corta cascada:

```text
tank
└─ turret [component]
   └─ projectile [normal]
```

projectile no muere sólo porque tank muera.

No existe borrado silencioso de components.

## 5. born()

Se ejecuta exactamente una vez cuando la instancia entra efectivamente en mundo.

Su identidad, mechanics, contexto de creación y effective State context ya están disponibles.

Puede usar operaciones Runtime, local/global, spawn y kill.

Self-kill durante born = nace y muere inmediatamente; no existe “cancelled birth”.

## 6. kill()

`kill(object)`:
- establece `alive=false` inmediatamente;
- lectura posterior válida del mismo hook observa false;
- excluye al objeto de todo procesamiento alive-only posterior, incluso misma fase/frame;
- no elimina físicamente todavía;
- programa la ruta normal dead/cleanup;
- es terminal;
- kill sobre ya muerto no duplica dead.

Si el objeto tiene component descendants vivos, se aplica la misma transición a ellos.

## 7. dead()

Se ejecuta exactamente una vez después de alive=false y antes de cleanup.

El `object` de dead es contexto final controlado según References.

Puede generar consecuencias como spawn/audio.

Spawn desde dead no se cancela por cleanup del source.

## 8. show()/hide()

Visibilidad es sólo participación visual.

Hidden:
- sigue alive;
- behavior/mechanics/Collision/State/timers continúan;
- no render.

`visible` es readonly; se muta con `show/hide`.

## 9. keep_only()

`keep_only(object)` conserva exactamente la instancia indicada y mata las demás instancias vivas mediante lifecycle normal.

No significa preservar automáticamente la entidad lógica completa del objeto.

Por tanto, salvo cambio explícito futuro, components del objeto conservado cuentan como “otras instancias” y pueden morir por keep_only.

La muerte inducida por keep_only ejecuta dead normalmente y puede generar nuevos spawn.

## 10. Invariantes

- LIFE-001 born una vez.
- LIFE-002 kill hace alive=false inmediatamente.
- LIFE-003 dead una vez.
- LIFE-004 muerte terminal.
- LIFE-005 child normal independiente del parent.
- LIFE-006 component conserva lifecycle propio mientras parent vive.
- LIFE-007 kill de RuntimeObject propaga a component descendants vivos.
- LIFE-008 cascade usa dead/cleanup normal.
- LIFE-009 child normal corta cascade.
- LIFE-010 visibility no suspende Runtime.
- LIFE-011 spawn desde dead permanece válido.
- LIFE-012 spawn desde Collision no participa esa fase.
