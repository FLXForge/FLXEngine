# Creation

## Estado
- Documento: especificación normativa
- Ámbito: creación declarativa de RuntimeObjects
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Principio

Creation describe cómo un RuntimeObject materializa instancias a partir de sus declaraciones `children`.

```text
children
   │
   ▼
Creation
   │
   ▼
RuntimeObjects
```

Creation no describe el comportamiento posterior de las instancias ni el motivo de juego por el que existen.

> Creation materializa población declarada; Lifecycle gobierna la vida de cada instancia.

Los conceptos de juego no forman parte de Creation:

```text
enemy
player
life
respawn
wave
level
projectile
```

El Runtime sólo conoce declaraciones, instancias y políticas de creación.

## 2. Declaración

Un objeto puede declarar una política `creation`:

```json
"creation": {
  "mode": "iterator",
  "rules": {
    "concurrent": 1,
    "repeat": true
  },
  "pattern": [
    "tank"
  ]
}
```

`creation` actúa sobre las declaraciones contenidas en `children`.

```json
{
  "creation": {
    "mode": "iterator",
    "pattern": [
      "tank"
    ]
  },
  "children": {
    "tank": {
      "like": "tank/player"
    }
  }
}
```

`pattern` contiene nombres de declaraciones child, no ids de RuntimeObject, grupos ni rutas de recursos.

## 3. Modos

FLX 0.3.0 define tres modos:

```text
individual
grid
iterator
```

Conceptualmente:

```text
individual
    declaración → instancia

grid
    patrón + reglas espaciales → instancias

iterator
    patrón + reglas de población/secuencia → instancias
```

`grid` responde principalmente a:

> ¿Dónde y cuántas instancias se materializan espacialmente?

`iterator` responde principalmente a:

> ¿Qué instancia corresponde ahora y cuántas pueden mantenerse simultáneamente?

Los modos son políticas de Creation, no tipos distintos de RuntimeObject.

## 4. Individual

`individual` es el modo básico de creación.

Es el modo por defecto cuando no se declara otro modo:

```json
"creation": {
  "mode": "individual"
}
```

y `mode` omitido dentro de una declaración Creation equivale a `individual`.

La creación individual conserva la semántica normal de los children de FLX.

Un child con:

```json
"spawn": "auto"
```

se materializa automáticamente según el ciclo normal de entrada del parent.

`spawn: "auto"` es el valor por defecto.

Un child con:

```json
"spawn": "manual"
```

no se materializa automáticamente y puede solicitarse mediante:

```js
spawn(object, "childName");
```

Ejemplo:

```json
"children": {
  "laser": {
    "like": "laser",
    "spawn": "manual"
  }
}
```

```js
spawn(ship, "laser");
```

Individual no mantiene una población ni sustituye automáticamente una instancia que muere.

## 5. Grid

`grid` materializa children siguiendo un patrón bidimensional y unas reglas espaciales explícitas.

Ejemplo conceptual:

```json
"creation": {
  "mode": "grid",
  "rules": {
    "rows": 3,
    "columns": 5,
    "cellWidth": 24,
    "cellHeight": 18
  },
  "pattern": [
    ["a", "a", "a", "a", "a"],
    ["b", "b", "b", "b", "b"],
    ["c", "c", "c", "c", "c"]
  ]
}
```

Las reglas requeridas son:

```text
rows       integer >= 1
columns    integer >= 1
cellWidth  number > 0
cellHeight number > 0
```

Para `grid`, `rules` y `pattern` son obligatorios.

Grid interpreta `pattern` espacialmente. No es un Iterator bidimensional ni mantiene capacidad cuando las instancias mueren.

La muerte posterior de una instancia creada por Grid no provoca por sí misma la creación de otra.

## 6. Iterator

`iterator` materializa una secuencia ordenada de declaraciones child y mantiene una capacidad máxima de instancias producidas por ese proceso.

Ejemplo mínimo:

```json
"creation": {
  "mode": "iterator",
  "pattern": [
    "a",
    "b",
    "c"
  ]
}
```

Sus defaults son:

```text
concurrent = 1
repeat     = false
```

Por tanto el ejemplo anterior equivale conceptualmente a:

```json
"creation": {
  "mode": "iterator",
  "rules": {
    "concurrent": 1,
    "repeat": false
  },
  "pattern": [
    "a",
    "b",
    "c"
  ]
}
```

`pattern` es obligatorio y debe contener al menos una entrada.

## 7. Cursor de Iterator

Cada proceso Iterator posee su propio cursor sobre `pattern`.

Para:

```json
"pattern": [
  "a",
  "b",
  "c"
]
```

el orden lógico es:

```text
a → b → c
```

El cursor pertenece al proceso Creation, no a las instancias creadas.

La muerte de una instancia no hace retroceder el cursor.

## 8. Concurrent

`rules.concurrent` define el máximo de instancias simultáneamente vivas o pendientes producidas por ese Iterator.

Debe ser:

```text
integer >= 1
```

No representa:

```text
número total de creaciones
vidas
repeticiones
tamaño del pattern
```

Representa capacidad simultánea.

Ejemplo:

```json
"creation": {
  "mode": "iterator",
  "rules": {
    "concurrent": 2
  },
  "pattern": [
    "a",
    "b",
    "c"
  ]
}
```

Inicialmente:

```text
capacity = 2

cursor
  │
  ▼
[a, b, c]

materializa:
a
b
```

Mientras `a` y `b` permanezcan vivos, no se materializa `c`.

Cuando uno deja de ocupar capacidad:

```text
a   b
    ↓ muere

capacidad libre
    ↓
cursor → c
    ↓
materializa c
```

quedando:

```text
a   c
```

`concurrent` no exige que las instancias sean del mismo tipo.

## 9. Pending cuenta como capacidad

Una creación solicitada por Iterator que todavía está pendiente de integración en el mundo cuenta para `concurrent`.

Conceptualmente:

```text
live + pending <= concurrent
```

Esto evita que varias evaluaciones anteriores a la integración estable puedan sobrepasar la capacidad declarada.

Por tanto, Iterator mantiene capacidad sobre sus instancias vivas y pendientes, no sólo sobre las que ya aparecen en consultas Runtime normales.

## 10. Repeat

`rules.repeat` controla qué ocurre cuando el cursor alcanza el final de `pattern`.

Default:

```text
repeat = false
```

### 10.1 Finito

Con:

```json
"rules": {
  "concurrent": 1,
  "repeat": false
}
```

y:

```text
[a, b, c]
```

el proceso produce:

```text
a
↓ muerte/liberación
b
↓ muerte/liberación
c
↓ muerte/liberación
fin
```

Una vez consumido el pattern, el cursor no vuelve al principio.

### 10.2 Repeating

Con:

```json
"rules": {
  "concurrent": 1,
  "repeat": true
}
```

el mismo pattern produce:

```text
a → b → c → a → b → c → ...
```

mientras exista el creador.

El wrap forma parte de la secuencia normal del Iterator.

## 11. Repeat y llenado inicial

`repeat` también se aplica durante el llenado de capacidad.

Ejemplo:

```json
"creation": {
  "mode": "iterator",
  "rules": {
    "concurrent": 3,
    "repeat": true
  },
  "pattern": [
    "a",
    "b"
  ]
}
```

produce inicialmente:

```text
a
b
a
```

No limita el llenado inicial al tamaño físico de `pattern`.

La capacidad y el cursor son conceptos independientes.

## 12. Iterator finito y finalización

Un Iterator finito no está terminado simplemente porque su cursor haya consumido todas las entradas.

Ejemplo:

```text
pattern = [a, b, c]
concurrent = 2
repeat = false
```

Estado:

```text
a   c
```

puede tener el cursor ya agotado, pero todavía existen instancias pertenecientes al proceso.

Por tanto:

```text
pattern agotado
≠
Creation terminada
```

El Iterator finito termina cuando se cumplen simultáneamente:

```text
pattern agotado
AND
ninguna instancia viva producida por el Iterator
AND
ninguna instancia pendiente producida por el Iterator
```

## 13. Pertenencia al Iterator

La capacidad de un Iterator se calcula exclusivamente sobre las instancias que pertenecen a ese proceso Creation.

No basta con contar:

```text
children del parent
objetos con el mismo name
objetos del mismo group
objetos creados desde la misma declaración
```

Una instancia creada manualmente mediante `spawn()` no pasa a pertenecer automáticamente a un Iterator sólo porque use la misma declaración child.

Conceptualmente, Runtime conserva la procedencia necesaria para distinguir:

```text
instancia producida por Iterator A
instancia producida por Iterator B
instancia producida manualmente
```

Esta procedencia es bookkeeping interno y no forma parte de `RuntimeObject` JS.

## 14. Iterator y children

Las entradas de `pattern` hacen referencia a nombres existentes en el mapa `children` del creador.

En FLX 0.3.0, las declaraciones usadas por Iterator deben ser children automáticos compatibles con la política Iterator.

Los children declarados como:

```json
"spawn": "manual"
```

no forman parte de un pattern Iterator válido.

Esta restricción es deliberada en v0.3.0.

`manual` expresa una creación solicitada explícitamente por comportamiento; Iterator expresa una política declarativa de población/secuencia.

## 15. Iterator no sustituye spawn()

Iterator y `spawn()` resuelven problemas distintos.

### Iterator

```text
"debe mantenerse/recorrerse esta población declarada"
```

### spawn()

```text
"como consecuencia de este comportamiento,
quiero crear ahora una instancia"
```

Ejemplo: una nave puede mantenerse mediante Iterator:

```json
"creation": {
  "mode": "iterator",
  "rules": {
    "concurrent": 1,
    "repeat": true
  },
  "pattern": [
    "ship"
  ]
}
```

pero sus disparos continúan siendo:

```js
spawn(ship, "laser");
```

y sus fragmentos al morir:

```js
spawn(ship, "fragment");
```

No toda creación dinámica pertenece a Iterator.

## 16. Iterator no es un generador temporal

Iterator no define intervalos, delays, probabilidades, oleadas ni condiciones arbitrarias de gameplay.

Un comportamiento como:

```text
cada 1.5 segundos
si hay menos de 10 asteroides
elige aleatoriamente A/B/C
crea uno
```

no equivale a:

```text
iterator [A,B,C]
```

El primero contiene semántica temporal y probabilística del juego.

El segundo contiene una secuencia y una política de capacidad.

En v0.3.0, la lógica temporal/probabilística permanece en scripting cuando pertenece al comportamiento del juego.

## 17. Relación con Lifecycle

Creation decide cuándo debe materializarse una nueva instancia.

Lifecycle decide cuándo una instancia deja de estar viva y cuándo se retira del mundo.

```text
Creation
   ↓
instancia
   ↓
Lifecycle
   ↓
kill()
   ↓
dead()
   ↓
cleanup
```

Iterator observa posteriormente la capacidad resultante y puede solicitar otra materialización.

La muerte no es un evento especial de Iterator.

Iterator no redefine `kill()`, `dead()` ni cleanup.

## 18. Sustitución tras muerte

Para:

```json
"creation": {
  "mode": "iterator",
  "rules": {
    "concurrent": 1,
    "repeat": true
  },
  "pattern": [
    "tank"
  ]
}
```

la secuencia conceptual es:

```text
tank vivo
   ↓
kill(tank)
   ↓
tank deja de estar vivo
   ↓
dead(tank)
   ↓
Iterator observa capacidad libre
   ↓
solicita siguiente "tank"
   ↓
cleanup de la instancia anterior
   ↓
integración estable
   ↓
born(nuevo tank)
```

La sustitución no es una creación síncrona ejecutada dentro de `kill()` o `collision()`.

Las nuevas instancias respetan el pipeline normal de integración del Runtime.

## 19. Identidad de instancias

Cada materialización produce una instancia Runtime nueva.

Por tanto:

```text
tank #1
kill
tank #2
```

implica:

```text
tank#1.id != tank#2.id
```

aunque ambas procedan de la misma declaración:

```text
name == "tank"
```

`name` representa identidad lógica/declarativa.

`id` representa identidad concreta de instancia.

Iterator no resucita RuntimeObjects muertos.

## 20. Posición y declaración child

Iterator selecciona y materializa declaraciones child normales.

No redefine su semántica espacial.

Ejemplo:

```json
"player_spawn": {
  "origin": {
    "x": 90,
    "y": 160
  },
  "creation": {
    "mode": "iterator",
    "rules": {
      "concurrent": 1,
      "repeat": true
    },
    "pattern": [
      "tank"
    ]
  },
  "children": {
    "tank": {
      "like": "tank/player"
    }
  }
}
```

El `tank` se materializa usando las reglas espaciales normales de la relación parent/child.

Creation no introduce una noción independiente de spawn point.

La posición pertenece al modelo espacial normal de los RuntimeObjects y sus declaraciones.

## 21. API `creation_active`

Scripting dispone de:

```ts
creation_active(object: RuntimeObject): boolean;
```

Consulta si el proceso Creation Iterator del objeto continúa activo.

No significa:

```text
"este objeto tiene un bloque creation"
```

ni:

```text
"este objeto ha creado alguna vez algo"
```

Significa:

> El proceso Iterator de Creation de este RuntimeObject continúa activo.

## 22. `creation_active` en Individual y Grid

Para:

```text
individual
grid
```

devuelve:

```js
false
```

`creation_active()` en v0.3.0 es una consulta sobre actividad Iterator, no una consulta genérica de presencia de configuración Creation.

## 23. `creation_active` en Iterator repeating

Para:

```json
"rules": {
  "repeat": true
}
```

devuelve:

```js
true
```

mientras exista el creador y su proceso Iterator esté vigente.

No depende de que en ese instante concreto haya capacidad libre.

## 24. `creation_active` en Iterator finito

Para un Iterator con:

```json
"rules": {
  "repeat": false
}
```

permanece `true` mientras:

```text
queden entradas por consumir
OR
existan instancias vivas producidas por el Iterator
OR
existan instancias pendientes producidas por el Iterator
```

Sólo pasa a `false` cuando:

```text
pattern agotado
AND
live == 0
AND
pending == 0
```

Ejemplo:

```text
pattern = [A,B,C]
concurrent = 2
repeat = false
```

Evolución:

```text
A B       creation_active = true
A C       creation_active = true
  C       creation_active = true
  ∅       creation_active = false
```

El agotamiento del cursor por sí solo no desactiva Creation.

## 25. `creation_active` no es un evento

`creation_active()` es una consulta de estado persistente.

No es:

```text
callback
evento
edge trigger
```

Por tanto:

```js
if (!creation_active(object)) {
    // ...
}
```

será cierto en todos los hooks posteriores mientras esa situación permanezca.

Si el juego necesita ejecutar una reacción una sola vez, debe conservar su propio estado o usar los mecanismos Runtime apropiados.

Creation no mata automáticamente al creador al terminar.

## 26. Ejemplo: Tank

Tank utiliza dos Iterator independientes:

```text
game
├── player_spawn
│   └── iterator [tank]
│
└── enemy_spawn
    └── iterator [tank]
```

Cada uno declara:

```json
"creation": {
  "mode": "iterator",
  "rules": {
    "concurrent": 1,
    "repeat": true
  },
  "pattern": [
    "tank"
  ]
}
```

El resultado es:

```text
player vivo
enemy vivo
    ↓
uno muere
    ↓
su Iterator libera capacidad
    ↓
nueva instancia
```

Ni `player` ni `enemy` conocen el concepto de respawn.

Los proyectiles siguen siendo children manuales creados mediante `spawn()`.

Esto permite expresar un duelo perpetuo sin introducir `lives`, `respawn` ni lógica de reposición en los scripts de los tanques.

## 27. Ejemplo: Asteroids

Asteroids utiliza Iterator para mantener la nave:

```text
ship_spawn
    ↓
iterator
concurrent = 1
repeat = true
pattern = [ship]
```

La nave no necesita comunicar su muerte al objeto `game` ni éste comprobar cada frame si debe crear otra.

Antes, conceptualmente:

```text
ship muere
   ↓
game guarda "no hay ship"
   ↓
game comprueba estado
   ↓
spawn(ship)
```

Con Iterator:

```text
ship muere
   ↓
capacidad libre
   ↓
Creation
   ↓
nueva ship
```

Sin embargo, los asteroides conservan su generación mediante scripting porque su creación depende de:

```text
timer
límite global
probabilidad
selección A/B/C
```

Ese comportamiento no representa una secuencia Iterator.

Asteroids muestra por tanto las dos fronteras:

```text
población persistente declarativa
    → Iterator

generación temporal/probabilística de gameplay
    → scripting
```

## 28. Composición de políticas

Un RuntimeObject posee una única política `creation`.

FLX 0.3.0 no define un array de políticas Creation dentro del mismo objeto.

Cuando el mundo necesita políticas independientes, se expresan mediante composición normal de RuntimeObjects.

Ejemplo:

```text
game
├── player_spawn
│      creation: iterator
│
└── enemy_spawn
       creation: iterator
```

No:

```json
"creation": [
  { "...": "..." },
  { "...": "..." }
]
```

La estructura del mundo permite separar procesos de Creation sin introducir un sistema paralelo de múltiples políticas.

## 29. Creation y estructura

Las instancias materializadas siguen siendo children estructurales normales.

Creation no introduce:

```text
ownership alternativo
parent alternativo
entidad lógica nueva
jerarquía paralela
```

La navegación pública continúa usando:

```ts
find_parent(object)
find_children(object)
```

según el contrato normal de References.

La procedencia Iterator necesaria para bookkeeping interno no modifica el parent estructural público.

## 30. Creation y RuntimeObject JS

Creation no añade propiedades al `RuntimeObject` público.

No existen propiedades como:

```text
object.creation
object.creator
object.iterator
object.respawn
object.lives
object.creationIndex
```

La superficie pública continúa siendo la definida por `RuntimeObject`.

La consulta específica necesaria es una operación Runtime:

```js
creation_active(object)
```

Esto mantiene la regla general:

```text
RuntimeObject observa estado local habitual
operaciones Runtime expresan relaciones/capacidades
```

## 31. Validación general

`creation` debe cumplir el schema correspondiente.

Modos válidos en v0.3.0:

```text
individual
grid
iterator
```

Los tipos son estrictos.

No se realizan coerciones conceptuales entre modos.

Un campo perteneciente a las reglas de otro modo no adquiere significado por estar presente.

Las restricciones específicas del modo deben validarse antes de ejecutar el mundo.

## 32. Validación de Iterator

Para Iterator:

```text
pattern
```

es obligatorio, debe ser un array plano no vacío y sus entradas deben ser strings que referencien declaraciones child válidas para Iterator.

Inválidos conceptualmente:

```json
"pattern": []
```

```json
"pattern": [
  ["a", "b"]
]
```

```json
"rules": {
  "concurrent": 0
}
```

```json
"rules": {
  "repeat": 1
}
```

También es inválido usar en el pattern una declaración child manual en v0.3.0.

## 33. Validación de Grid

Para Grid son obligatorios:

```text
rules
pattern
rows
columns
cellWidth
cellHeight
```

con:

```text
rows >= 1
columns >= 1
cellWidth > 0
cellHeight > 0
```

Grid conserva su contrato previo; Iterator no modifica sus defaults ni semántica.

## 34. Estabilidad durante actualización

Creation no debe invalidar la iteración activa sobre la colección Runtime mediante inserciones estructurales inseguras.

Las materializaciones solicitadas durante mantenimiento se integran mediante el mecanismo estable del Runtime.

Esto forma parte del contrato observable de orden y lifecycle:

```text
estado vivo
    ↓
Creation decide
    ↓
pending
    ↓
integración estable
    ↓
born
```

No debe aparecer una instancia parcialmente integrada como consecuencia de modificar directamente la colección activa durante su recorrido.

## 35. Serialización

La configuración Creation forma parte del formato compilado FLX.

La incorporación de Iterator en v0.3.0 actualiza el formato binario a:

```text
FLXC version 12
```

La serialización debe preservar la información necesaria para reconstruir:

```text
mode
rules
pattern
```

sin alterar la semántica declarada.

El formato compilado y el JSON fuente deben producir el mismo comportamiento Runtime para Creation.

## 36. Qué no pertenece a Creation v0.3.0

Fuera del contrato Creation v0.3.0:

```text
respawn como concepto propio
lives
waves
spawn delay
spawn interval
probabilidad declarativa
random pattern
regiones de aparición
safe spawn / occupancy
condiciones arbitrarias de gameplay
callbacks de finalización
creation_finished()
auto-kill del creador
múltiples políticas Creation en array
pattern Iterator con children manuales
```

Estas ausencias no implican necesariamente futuras APIs.

Una necesidad futura debe evaluarse desde el juego y no extrapolarse automáticamente a Creation.

## 37. Frontera conceptual

Creation debe absorber aquello que pertenece a la materialización declarativa del mundo, pero no la lógica específica del juego.

Ejemplos:

```text
"mantén un tanque"
    → Iterator

"recorre A,B,C"
    → Iterator

"forma esta matriz"
    → Grid

"dispara un proyectil ahora"
    → spawn()

"crea un asteroide cada 1.5 s con probabilidad"
    → scripting

"el jugador tiene tres vidas"
    → gameplay, no Creation
```

El criterio no es si algo crea objetos.

El criterio es **qué significado tiene esa creación**.

## 38. Invariantes

- CRE-001 Creation materializa declaraciones `children` como RuntimeObjects.
- CRE-002 Creation no introduce conceptos específicos de gameplay.
- CRE-003 `individual`, `grid` e `iterator` son políticas de Creation, no tipos de RuntimeObject.
- CRE-004 `individual` conserva la semántica normal auto/manual de children.
- CRE-005 Grid expresa un patrón espacial y conserva su contrato previo.
- CRE-006 Iterator posee un cursor ordenado propio sobre un pattern plano no vacío.
- CRE-007 `concurrent` limita instancias Iterator vivas más pendientes, no el número total de creaciones.
- CRE-008 Pending cuenta para capacidad Iterator.
- CRE-009 `repeat=false` no reinicia el cursor.
- CRE-010 `repeat=true` permite wrap también durante el llenado inicial.
- CRE-011 Un Iterator finito termina sólo con pattern agotado y sin instancias vivas ni pendientes propias.
- CRE-012 La pertenencia a Iterator es procedencia Runtime interna, no se deduce de name, group o parent.
- CRE-013 `spawn()` manual no convierte una instancia en miembro de Iterator.
- CRE-014 Iterator v0.3.0 sólo referencia children compatibles con creación automática.
- CRE-015 La muerte pertenece a Lifecycle; Iterator observa posteriormente la capacidad disponible.
- CRE-016 La sustitución no ocurre síncronamente dentro de `kill()` ni de `collision()`.
- CRE-017 Cada materialización produce una identidad Runtime nueva.
- CRE-018 Iterator no redefine posición ni la relación espacial parent/child.
- CRE-019 `creation_active()` devuelve false para individual y grid.
- CRE-020 Iterator repeating permanece activo mientras exista su proceso creador.
- CRE-021 Iterator finito permanece activo hasta agotar pattern, vivos y pendientes.
- CRE-022 `creation_active()` es consulta persistente, no evento.
- CRE-023 Creation no mata automáticamente a su creador.
- CRE-024 Un RuntimeObject posee una única política Creation; varias políticas se componen estructuralmente.
- CRE-025 Creation no altera la jerarquía pública ni la navegación References.
- CRE-026 Creation no amplía la superficie de propiedades de RuntimeObject JS.
- CRE-027 Iterator no sustituye generación temporal, probabilística o causal de gameplay.
- CRE-028 JSON y FLXC deben conservar semántica Creation equivalente.