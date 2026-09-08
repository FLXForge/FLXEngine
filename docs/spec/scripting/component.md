# Component

## Estado
- Documento: especificación normativa
- Ámbito: composición lógica de RuntimeObjects
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Propósito

FLX no define tipos públicos distintos para nodo, entidad, UI o componente. Todo objeto vivo es un `RuntimeObject`; su papel emerge de su configuración.

Un hijo puede declarar en la raíz de su JSON:

```json
"component": true
```

Esto significa que ese RuntimeObject es una parte lógica de su `parent` inmediato, no una entidad lógica completa independiente.

`component` no crea una clase distinta ni reduce capacidades.

> Un componente no tiene menos capacidades que otro RuntimeObject; tiene menos independencia.

## 2. Declaración

`component` es una propiedad raíz, opcional y booleana del **child**:

```json
{
  "component": true
}
```

Reglas:
- ausencia = RuntimeObject completo / entidad lógica propia;
- `component:false` es válido pero no necesario en forma canónica;
- el parent es implícitamente el parent estructural inmediato;
- no existe `component.of` ni referencia por id/nombre;
- un root no puede declarar `component:true`, porque no tiene parent;
- no hay coerción de tipos.

## 3. Tipología emergente

```text
mismo RuntimeObject
+
configuración diferente
=
papel diferente
```

`component`, `children`, `shape`, `mechanics`, `collisions`, `states`, scripting o audio añaden capacidades/relaciones, no clases públicas distintas.

## 4. Capacidades

Un componente puede tener todas las capacidades normales de RuntimeObject:

```text
scripts
mechanics
shape
collisions
children
states
local state
audio
```

## 5. Jerarquía, Component y Attach

Son ortogonales:

```text
parent / children
→ estructura y contexto de creación

component
→ pertenencia a una misma entidad lógica

attach
→ dependencia espacial viva
```

`component` no implica `attach`.
`attach` no convierte un RuntimeObject en componente.

## 6. Entidad lógica y cadenas

La pertenencia por Component es transitiva mientras la cadena continúe mediante RuntimeObjects con `component:true`.

```text
tank
└─ turret [component]
   └─ cannon [component]
```

Los tres forman una misma entidad lógica completa.

Un child normal corta la continuidad:

```text
tank
└─ turret [component]
   └─ projectile [normal child]
```

`projectile` constituye una entidad lógica propia.

## 7. Lifecycle

Cada componente posee lifecycle propio mientras el parent está vivo.

Puede nacer, morir, ejecutar `dead()` y ser recreado mediante `spawn()` como cualquier RuntimeObject.

### 7.1. Muerte de un componente

```text
kill(cannon)
→ cannon muere
→ sus component descendants vivos también
→ turret y tank continúan vivos
```

### 7.2. Muerte del parent

La muerte de un RuntimeObject mata todos sus **component descendants vivos**, transitivamente.

```text
kill(tank)
→ tank
→ turret [component]
→ cannon [component]
```

Todos siguen el lifecycle normal:

```text
alive = false inmediatamente
→ dead()
→ cleanup
```

No hay borrado silencioso.

### 7.3. Child normal

Un child normal no muere automáticamente con su parent por jerarquía.

```text
hierarchy ≠ ownership
component = relación explícita de pertenencia/lifetime
```

## 8. Spawn

`component` no altera por sí mismo las fronteras temporales de `spawn()`.

Una declaración child component puede ser automática o manual según el contrato de creación existente.

## 9. Effective State context

Para cualquier RuntimeObject `O`:

```text
effectiveStateContext(O):
1. si O declara states → usar la StateMachine de O
2. si O no declara states y O es component → resolver parent
3. si O no es component → no StateMachine
```

Se detiene en la primera StateMachine encontrada o en el primer RuntimeObject no-component sin states.

Ejemplo:

```text
tank states = patrol / alert / attack
├─ body [component], sin states
│  → usa State de tank
└─ turret [component], states = idle / tracking / firing
   └─ cannon [component], sin states
      → usa State de turret
```

`state_to(cannon, "firing")` modifica la StateMachine viva de `turret`.

Un child normal no hereda State.

## 10. Collision

Component no colapsa identidades Collision.

Cada componente conserva:
- su RuntimeObject;
- su `group`;
- sus colliders;
- su callback `collision()`.

Si turret recibe el contacto:

```text
collision(turret, shot, contacts)
```

no se redirige a `tank`.

Collision excluye sólo el mismo RuntimeObject exacto; no excluye automáticamente otros miembros de la misma entidad lógica.

## 11. Ray

Ray sí excluye la entidad lógica completa del source.

```text
Collision → identidad RuntimeObject exacta
Ray       → observación desde entidad lógica
```

Un ray de turret no impacta colliders de tank ni de otros components de esa misma entidad.

Un child normal no queda excluido sólo por compartir jerarquía.

## 12. Multicollider vs Component

```text
multicollider
→ varias superficies
→ un RuntimeObject

component
→ varios RuntimeObjects
→ una entidad lógica
```

No deben confundirse.

## 13. Referencias

`find_parent()` y `find_children()` conservan estructura real y no colapsan componentes.

```text
find_parent(cannon) → turret
find_parent(turret) → tank
```

No se expone en v0.3 una referencia pública separada de “entidad lógica”.

## 14. Invariantes

- COMP-001 `component` es propiedad raíz booleana de un child.
- COMP-002 Ausencia de `component` significa entidad lógica propia por defecto.
- COMP-003 Root con `component:true` es inválido.
- COMP-004 Component no crea un tipo distinto de RuntimeObject.
- COMP-005 Un componente conserva todas las capacidades de RuntimeObject.
- COMP-006 Component, hierarchy y attach son conceptos distintos.
- COMP-007 La pertenencia Component es transitiva.
- COMP-008 Un child normal corta la continuidad de entidad lógica.
- COMP-009 Cada componente mantiene lifecycle propio mientras parent vive.
- COMP-010 Muerte de un RuntimeObject mata component descendants vivos.
- COMP-011 La cascada usa kill/dead/cleanup normal.
- COMP-012 Child normal no muere automáticamente por muerte del parent.
- COMP-013 Un componente con states propios inicia nuevo State context.
- COMP-014 Un componente sin states puede resolver State context ascendente.
- COMP-015 Child normal no hereda State context.
- COMP-016 Collision conserva identidad por RuntimeObject.
- COMP-017 Ray excluye la entidad lógica completa del source.
- COMP-018 Multicollider y Component son conceptos distintos.

## 15. Principio final

Component expresa pertenencia, no reducción de capacidades.

```text
RuntimeObject completo
├─ normal    → entidad lógica propia
└─ component → parte de la entidad lógica del parent
```
