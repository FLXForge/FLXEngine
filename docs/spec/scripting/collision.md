# Collision

## Estado
- Documento: especificación normativa
- Ámbito: Collision, colliders, contactos y consultas Ray
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Propósito
Collision describe las interacciones espaciales entre objetos vivos.

FLX separa tres conceptos:

```text
RuntimeObject
→ identidad y comportamiento vivo

Collider
→ geometría de interacción perteneciente a un RuntimeObject

Collision
→ relación dirigida entre RuntimeObjects a través de sus colliders
```

Los RuntimeObjects no colisionan directamente. Sus colliders producen contactos y esos contactos pueden provocar una interacción entre dos RuntimeObjects.

Collision detecta y describe. No interpreta qué significa el contacto ni aplica automáticamente respuesta física.

## 2. Principio fundamental
Collision en FLX es selectivo, dirigido, pasivo cuando es posible, geométricamente preciso e independiente del dibujo.

Un collider sólo inicia detección cuando declara explícitamente con qué grupos quiere interactuar. Los colliders que no declaran ninguna interacción pueden seguir siendo superficies válidas contra las que otros RuntimeObjects colisionan.

```text
source collider
    │ with: ["enemy"]
    ▼
candidate RuntimeObjects
    ▼
effective target colliders
    ▼
geometry
    ▼
CollisionContact[]
```

La existencia de un collider no implica que ese collider inicie trabajo de detección.

## 3. Declaración

```json
"collisions": {
    "body": {},
    "attack": {
        "type": "box",
        "size": { "width": 30, "height": 10 },
        "offset": { "x": 24 },
        "angle": 0,
        "with": ["enemy"],
        "enabled": false,
        "states": ["attack"]
    }
}
```

`collisions` contiene declaraciones nombradas. Cada entrada representa un collider perteneciente al RuntimeObject.

Los nombres de collider son locales al RuntimeObject. No son RuntimeObjects, recursos globales ni referencias buscables.

## 4. Campos de un collider
Campos válidos en 0.3.0:

```text
type
size
offset
angle
with
enabled
states
```

### 4.1 `type`
Tipos válidos: `box`, `ellipse`. Ausente: `box`.

No existen aliases públicos `circle`, `rectangle` ni `aabb`.

`box` representa un rectángulo orientable. No se reduce a AABB durante narrow phase.

`ellipse` representa una elipse orientable. Una elipse con `width == height` es un círculo.

No existe `radius`, `radiusX` ni `radiusY` como API pública de Collision.

## 5. Tamaño
Cada eje se resuelve independientemente. Si una dimensión no se declara, hereda la dimensión correspondiente del RuntimeObject.

```text
collider.width  = declared width  ?? object.width
collider.height = declared height ?? object.height
```

Collision nunca inventa geometría. Todo collider debe resolver `effective width > 0` y `effective height > 0`.

Un RuntimeObject no necesita Shape para tener collider.

```json
{
  "group": "checkpoint",
  "collisions": {
    "trigger": {
      "size": { "width": 64, "height": 8 }
    }
  }
}
```

## 6. Espacio local y transformación
La geometría Collision es independiente de Shape.

```text
RuntimeObject
├─ Shape
└─ Colliders
```

Puede imaginarse el RuntimeObject como un lienzo o post-it: shape y colliders son elementos locales fijados a ese lienzo; mover o rotar el RuntimeObject transforma sus elementos.

```text
world collider transform
=
object world transform
×
collider local transform
```

`offset` está expresado en espacio local del RuntimeObject y sitúa el centro geométrico del collider respecto al punto de referencia espacial estable del RuntimeObject.

```text
RuntimeObject reference
        ●──────────────● collider center
              offset
```

El significado exacto de `object.x / object.y` es una propiedad transversal del RuntimeObject y no se redefine dentro de Collision.

## 7. Rotación
`angle` es local al RuntimeObject.

```text
object angle + collider local angle
```

Admite cualquier número finito. Collision no exige normalización pública a `[0,360)`.

## 8. Group
`group` pertenece al RuntimeObject, no al collider. Es singular y representa su clasificación principal para interacción.

`group` no es tags.

Son válidos: group+colliders, group sin colliders, colliders sin group, ninguno.

```text
group    ≠ collider automático
collider ≠ group obligatorio
```

Un objeto sin `group` puede ser objetivo de Ray y aportar geometría pasiva.

## 9. Interacción dirigida: `with`
`with` declara los grupos contra los que un collider fuente puede iniciar Collision.

```text
A.collider.with includes B.group
→ A puede iniciar A → B
```

No implica `B → A`.

### 9.1 Collider pasivo
Ausencia o vacío de `with` significa collider pasivo.

```text
NO inicia búsquedas
SÍ puede ser objetivo
SÍ participa en Ray
```

si es efectivo.

## 10. Relaciones bidireccionales
Si ambos objetos declaran interacción, `A → B` y `B → A` son dos interacciones dirigidas independientes, no duplicados accidentales.

Si se evalúan sobre la misma geometría sin mutaciones intermedias, collider/otherCollider se intercambian, la normal se invierte, y punto/penetración se conservan.

No se garantiza que ambas callbacks ocurran: la primera puede modificar el mundo y la segunda se evalúa contra el nuevo estado vivo.

## 11. Exclusión propia
Un RuntimeObject nunca es candidato Collision de sí mismo. Colliders distintos del mismo RuntimeObject pueden solaparse sin generar `collision(A,A,...)`.

## 12. Componentes
Component no fusiona identidades Collision. Cada componente sigue siendo un RuntimeObject independiente.

Collision excluye únicamente el mismo RuntimeObject exacto. No excluye automáticamente otros RuntimeObjects de la misma entidad lógica.

```text
Collision → excluye sólo el RuntimeObject fuente
Ray       → excluye la entidad lógica completa del source
```

## 13. Enabled
Todo collider declarado comienza `enabled = true`, salvo declaración explícita `false`.

Un collider disabled no inicia, no es target, no entra en narrow phase y no puede ser Ray hit.

## 14. Estado y disponibilidad efectiva
`states` restringe un collider al State vivo actual. `states: []` es inválido.

Cada nombre debe existir en el effective State context. Los components pueden resolver el State context del parent según la especificación de State/Component.

```text
effective = enabled AND state condition satisfied
```

`enabled=true` y `effective=false` es un estado válido.

## 15. Runtime API

```ts
collider_on(object, colliderName): void
collider_off(object, colliderName): void
```

Son idempotentes y modifican exclusivamente `enabled`.

Scripting no puede crear colliders dinámicamente. Operar sobre un collider no declarado es inválido y debe seguir la política general de errores de scripting.

## 16. Multicollider
Un RuntimeObject puede tener varios colliders. Multicollider significa varias superficies de una misma identidad RuntimeObject; no equivale a composición.

```text
multicollider → misma identidad, varios colliders
component     → RuntimeObjects distintos
```

## 17. CollisionContact

```ts
interface CollisionContact {
    readonly collider: string;
    readonly otherCollider: string;
    readonly normalX: number;
    readonly normalY: number;
    readonly pointX: number;
    readonly pointY: number;
    readonly penetration: number;
}
```

`collider` identifica el collider de `object`; `otherCollider`, el de `other`.

## 18. Normal
La normal se expresa desde la perspectiva de `object`: dirección normalizada razonable en la que `object` podría desplazarse para salir de `other`.

```text
pared izquierda   → ( 1,  0)
pared derecha     → (-1,  0)
suelo / paddle    → ( 0, -1)
contacto inferior → ( 0,  1)
```

En esquinas/curvas puede ser diagonal normalizada. La perspectiva inversa invierte la normal.

## 19. Punto de contacto
`pointX/Y` representan un punto de contacto estable y perceptualmente natural en coordenadas de mundo.

No se promete manifold físico completo. Cuando el contacto ocupa una región/segmento, puede representarse razonablemente mediante un punto central.

## 20. Penetración
`penetration >= 0` y es coherente con la normal.

Conceptualmente, desplazar `object` en `normal * penetration` debería constituir una separación razonable para las primitivas soportadas.

Collision no aplica automáticamente ese desplazamiento.

## 21. Contacto por pareja de colliders
Una pareja concreta `A.colliderX × B.colliderY` produce como máximo un `CollisionContact` por evaluación.

## 22. Dispatch

```ts
collision(
    object: RuntimeObject,
    other: RuntimeObject,
    contacts: CollisionContact[]
): void
```

El dispatch se agrega por interacción dirigida entre RuntimeObjects. No se llama una vez por pareja de colliders.

## 23. Qué contactos pertenecen a `contacts[]`
Sólo contactos generados por colliders fuente cuyo `with` incluye `other.group`.

```text
A.body.with   = ["world"]
A.attack.with = ["enemy"]
B.group       = enemy

attack × body   ✓
attack × shield ✓
body × body     ✗
body × shield   ✗
```

El collider target no necesita `with`.

## 24. Varios colliders fuente
Si varios colliders fuente declaran relación hacia el mismo group, sus contactos se agregan en una sola callback RuntimeObject→RuntimeObject.

## 25. Varios RuntimeObjects del mismo group
`with:["enemy"]` clasifica candidatos; no significa un único enemy. Cada RuntimeObject target produce su propia interacción dirigida.

## 26. Orden
El resultado geométrico debe ser determinista para el mismo estado vivo y secuencia previa.

El orden entre interacciones independientes y el orden de `contacts[]` no tienen significado semántico público.

La implementación debe procurar estabilidad para tests/debug, pero los scripts no deben depender de `contacts[0]` como prioridad salvo que sólo pueda existir un contacto relevante.

## 27. Geometría efectiva
Debe existir una única representación efectiva de cada collider vivo:

```text
local declaration
→ RuntimeObject transform
→ EffectiveCollider
   ├─ broad-phase bounds
   ├─ narrow phase
   ├─ Ray
   └─ Debug
```

Collision, Ray y Debug no reconstruyen geometrías independientes.

Cambios de posición, size, angle, State o enabled deben actualizar/invalidate lo necesario antes de una interacción posterior dependiente.

## 28. Broad phase y narrow phase
Broad phase descarta candidatos imposibles y puede usar AABB u otras aproximaciones. No determina por sí sola un contacto público.

Narrow phase usa geometría efectiva real:

```text
box × box
box × ellipse
ellipse × box
ellipse × ellipse
```

Box rotada = OBB real. Ellipse = elipse orientada real, no circle/AABB como aproximación final.

## 29. Geometry layer
Conceptualmente:

```text
detect(geometryA, geometryB)
→ no contact
or
→ normal + point + penetration
```

Esta capa no conoce group, with, State, enabled, scripts, Component ni Machine.

## 30. Pipeline de Collision

```text
directed source collider
→ effective?
→ candidate acquisition
→ spatial compatibility
→ directed interaction filtering
→ broad phase
→ narrow phase
→ CollisionContact aggregation
→ collision()
```

El orden interno exacto puede optimizarse sin cambiar semántica observable.

## 31. Eficiencia
El coste debe tender a:

```text
active directed sources × spatially relevant candidates
```

no al total de colliders existentes.

Colliders sin `with` no inician detección. Disabled/state-unavailable se descartan antes de geometría innecesaria.

La estrategia de candidatos es interna y reemplazable: linear scan → spatial hash/grid/tree → índices futuros por espacio Machine.

## 32. Frontera QuickJS

```text
candidate acquisition
→ filtering
→ broad phase
→ narrow phase
→ contacts aggregation
→ QuickJS collision()
```

Multicollider no multiplica llamadas JS: una interacción dirigida RuntimeObject→RuntimeObject produce como máximo una callback por evaluación.

## 33. Atomicidad de una interacción dirigida
La unidad atómica es `source RuntimeObject → other RuntimeObject`, no un encuentro bidireccional.

```text
1. resolver estado vivo actual
2. obtener colliders relevantes efectivos
3. calcular todos los contactos de esa interacción
4. construir contacts[]
5. llamar collision(object, other, contacts)
6. hacer visibles las mutaciones para interacciones posteriores
```

Una interacción ya entregada no se recalcula retroactivamente.

## 34. Mutaciones durante Collision
Las operaciones realizadas dentro de `collision()` afectan a interacciones posteriores de la misma fase.

### 34.1 kill
`kill()` hace `alive=false` inmediatamente. Desde ese instante el RuntimeObject no participa en procesamiento activo posterior. Antes de dispatch, source y other deben seguir vivos.

### 34.2 collider_on/off
No modifican `contacts[]` ya entregados; afectan interacciones posteriores.

### 34.3 state_to
No recalcula la interacción actual; las siguientes observan el nuevo State vivo.

### 34.4 Transformación
Cambios de position/angle/size u otras propiedades vivas no alteran contactos ya entregados; interacciones posteriores usan la geometría actualizada.

## 35. Spawn durante Collision
Spawn desde `collision()` se integra al terminar la fase Collision. La nueva instancia nunca participa en esa misma fase.

## 36. Respuesta
Collision no resuelve automáticamente contactos.

Fuera de 0.3.0: `resolve()`, solids, plataformas, materiales, separación automática.

La respuesta pertenece al juego: reflect, kill, spawn, score, etc.

## 37. Machine y compatibilidad espacial
Collision no deduce compatibilidad a partir del `layer` visual.

La arquitectura reserva una etapa independiente `spatial compatibility`, potencialmente gobernable por Machine en versiones futuras.

En 0.3.0, salvo regla Machine explícita existente, la compatibilidad espacial es universal.

No se añade `collisionLayer`, `collisionPlane`, `space` ni `mask`.

## 38. Ray
Ray es consulta espacial explícita:

```text
Collision → ¿con quién declaré interacción?
Ray       → ¿qué geometría existe en esta trayectoria?
```

```ts
interface RayHit {
    readonly object: RuntimeObject;
    readonly collider: string;
    readonly pointX: number;
    readonly pointY: number;
    readonly normalX: number;
    readonly normalY: number;
    readonly distance: number;
}

ray(source: RuntimeObject, angle: number, distance: number): RayHit | undefined
```

## 39. Semántica de Ray
Parte del punto de referencia espacial estable del source, usa la convención angular FLX, recorre como máximo distance y devuelve el collider efectivo más cercano.

Usa la misma EffectiveCollider que Collision. No usa `with`, no dispara `collision()` y el target no necesita ser fuente activa.

## 40. Exclusión propia de Ray
Ray ignora todos los colliders de la misma entidad lógica completa que source, incluyendo components pertenecientes a ella.

Un child normal no-component es entidad independiente y no se excluye sólo por jerarquía.

## 41. Resultado Ray
Miss → `undefined`.

Devuelve point, normal exterior del target, distance origen→impacto y collider id. Contacto tangencial cuenta como hit.

Empates exactos deben ser estables internamente; la política no es pública.

## 42. Parámetros Ray

```text
distance > 0 → consulta normal
distance = 0 → undefined
distance < 0 → operación inválida
```

Angle debe ser finito. Source debe ser RuntimeObject vivo válido.

## 43. Debug
`debug.collisions=true` representa el estado y resultados reales del subsistema espacial.

> Debug observa Collision y Ray; no los reimplementa.

```text
EffectiveCollider ─► Collision
        ├──────────► Ray
        └──────────► Debug
```

## 44. Información de debug
Debe poder distinguir/representar:

```text
collider declared + effective
collider declared + disabled
collider declared + State unavailable
contacts generated
rays executed
```

No se congelan colores ni estilo visual.

## 45. Contact debug
Point y normal deben proceder del `CollisionContact` realmente generado, no de un recálculo posterior.

## 46. Ray debug
Debe representar origin, trajectory, hit y hit normal de las consultas realmente ejecutadas. Debug no ejecuta un segundo `ray()`.

## 47. Debug semántico vs interno
`debug.collisions` no necesita exponer AABB broad phase, buckets, candidate counts ni índices internos. Eso puede pertenecer a tooling de rendimiento futuro.

## 48. Validación
`collisions` debe ser object. Tipos estrictos, sin coerción.

Inválidos: `enabled:1`, `with:"enemy"`, `states:"attack"`.

## 49. Validación de with
Cada elemento string no vacío. `with:[]` válido. Duplicados inválidos.

Los grupos mencionados no necesitan existir en compilación: group es clasificación abierta, no registro cerrado.

## 50. Validación de states
Si existe: array no vacío, strings no vacíos, sin duplicados, debe existir effective State context y cada State debe existir allí.

## 51. Validación geométrica
Offset/angle finitos. Effective width/height positivos. Cero, negativos, NaN o Infinity inválidos.

La validación puede combinar dimensiones propias del collider y del RuntimeObject.

## 52. Colliders declarados
Scripting sólo opera sobre colliders declarados. JSON/definición declara capacidad; Runtime modifica enabled/effective.

## 53. Visibilidad
`visible=false` no altera Collision ni Ray. Un objeto oculto puede seguir vivo y espacialmente interactivo.

## 54. Spatial relevance
Off-screen no significa non-collidable.

```text
visibility → representación
enabled/State → existencia efectiva
spatial relevance → coste de una consulta concreta
```

## 55. Determinismo
Misma geometría efectiva → resultado estable de contact/no-contact, normal, point, penetration.

No se exponen epsilon, solver iterations, manifold order ni broad-phase order.

## 56. Evolución interna
Candidate provider, broad phase, geometry algorithms, spatial index, caches y debug representation pueden sustituirse sin cambiar el contrato.

## 57. Deliberadamente fuera de 0.3.0

```text
automatic physical resolution
solids
platforms
one-way surfaces
gravity collision response
collision materials
friction
restitution
collision masks
collision layers
all-hits ray
filtered ray
custom ray origin
include-self ray
dynamic collider creation
public Collider object
contact manifolds
continuous collision detection
```

## 58. Invariantes
- COL-001 Los RuntimeObjects interactúan espacialmente mediante Colliders.
- COL-002 Collider no es RuntimeObject ni identidad pública independiente.
- COL-003 `collisions` puede contener múltiples colliders nombrados.
- COL-004 `box` y `ellipse` son las geometrías públicas de 0.3.0.
- COL-005 Collision geometry es independiente de Shape.
- COL-006 Todo collider debe resolver tamaño efectivo positivo.
- COL-007 `with` declara interacción dirigida.
- COL-008 Ausencia o vacío de `with` produce collider pasivo.
- COL-009 Un collider pasivo no inicia detección.
- COL-010 El target no necesita declarar relación recíproca.
- COL-011 Un RuntimeObject nunca colisiona consigo mismo.
- COL-012 Component no introduce exclusión Collision implícita.
- COL-013 `enabled=false` hace al collider no efectivo.
- COL-014 La disponibilidad efectiva combina enabled y condición State.
- COL-015 `collider_on/off` sólo opera sobre colliders declarados.
- COL-016 Multicollider no crea nuevas identidades RuntimeObject.
- COL-017 Una pareja concreta de colliders produce como máximo un Contact.
- COL-018 Una interacción dirigida RuntimeObject→RuntimeObject produce como máximo una callback por evaluación.
- COL-019 `contacts[]` sólo contiene contactos autorizados por los colliders fuente.
- COL-020 El orden de `contacts[]` no tiene significado semántico público.
- COL-021 El orden entre interacciones independientes no tiene significado semántico público.
- COL-022 Una interacción dirigida es geométricamente atómica.
- COL-023 Las mutaciones de una callback afectan interacciones posteriores.
- COL-024 `kill()` excluye inmediatamente al RuntimeObject de interacciones posteriores.
- COL-025 Spawn desde Collision no participa en esa misma fase.
- COL-026 Collision no aplica respuesta física automática.
- COL-027 Broad phase no sustituye narrow phase.
- COL-028 Box y ellipse respetan transformación efectiva real.
- COL-029 Collision, Ray y Debug consumen la misma geometría efectiva.
- COL-030 Ray no utiliza `with`.
- COL-031 Ray devuelve el impacto efectivo más cercano.
- COL-032 Ray excluye la entidad lógica completa del source.
- COL-033 Collision excluye sólo el RuntimeObject fuente exacto.
- COL-034 Visibilidad no altera Collision.
- COL-035 Render layer no determina compatibilidad Collision.
- COL-036 La compatibilidad espacial pertenece a una frontera independiente, potencialmente gobernable por Machine.
- COL-037 Debug representa datos reales y no recalcula Collision o Ray.
- COL-038 La estrategia de candidatos es interna y reemplazable.
- COL-039 Colliders pasivos no deben convertirse en fuentes de coste por frame.
- COL-040 QuickJS recibe interacciones después de detección y agregación.

## 59. Tests mínimos
Cubrir al menos: collider mínimo; box/ellipse; size explícito/heredado por eje; offset/angle; multicollider; validación de type/size/with/states/enabled; dirección A→B y reciprocidad; self exclusion; Component; State efectivo; collider_on/off; kill inmediato; spawn durante collision; mutaciones geométricas; normal/point/penetration; agregación; Ray nearest/miss/zero/negative/rotated/ellipse/disabled/state/component exclusion; Debug basado en datos reales; colliders pasivos no iniciando consultas; una callback JS por interacción RuntimeObject→RuntimeObject.

## 60. Microejemplos
Los microejemplos incluidos en el paquete de auditoría complementan esta spec y pueden convertirse en tests o documentación ejecutable.

## 61. Principio final
Collision no representa física. Representa posibilidades geométricas de interacción declaradas por el juego.

```text
RuntimeObject
→ declara geometría posible
→ declara intención dirigida
→ Collision selecciona / detecta / describe
→ collision(object, other, contacts)
→ el comportamiento decide qué significa
```

La geometría informa. La declaración selecciona. El Runtime mantiene el mundo vivo. El script decide la reacción.
