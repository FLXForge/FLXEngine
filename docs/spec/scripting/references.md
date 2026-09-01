# References / Context / hierarchy

## Estado
Contrato de scripting de FLX v0.3.0 para referencias Runtime, navegación estructural y su relación con Mechanics relativas.

## Principio
`RuntimeObject` es la referencia pública de JavaScript hacia una instancia del Runtime. No es una copia de C++, espejo del JSON, handle persistente ni estado serializable.

> El scripting conserva identidad; el Runtime conserva el mundo vivo. Los RuntimeObjects se resuelven, no se persisten.

Un hook recibe un punto de vista concreto sobre el mundo. `obj` es el punto de partida del comportamiento, no su frontera.

## Vigencia
Un RuntimeObject normal es utilizable mientras pertenece a la invocación activa y su instancia sigue viva. La vigencia es por hook, no por frame. Al terminar la invocación, el wrapper caduca aunque la instancia continúe viva. Si la instancia muere durante el hook, la referencia normal caduca inmediatamente.

Para conservar identidad, se lee `object.id` mientras la referencia es válida, se guarda el string y se vuelve a resolver con `find_id()`.

## `dead(object)`
`dead(object)` es una excepción controlada: Runtime entrega deliberadamente el contexto final de la instancia muerta. Ese `object` puede utilizarse durante ese hook para estado final, `spawn`, audio y navegación estructural. La excepción sólo afecta al objeto de contexto del `dead` actual. Los resultados de `find_*` siguen siendo vivos.

## ScriptValue
`ScriptValue` sigue limitado a `number | boolean | string`. RuntimeObject no se persiste en local/global.

## Familia find
```ts
find_id(id: string): RuntimeObject | undefined;
find_name(name: string): RuntimeObject[];
find_parent(object: RuntimeObject): RuntimeObject | undefined;
find_children(object: RuntimeObject): RuntimeObject[];
```

`find_*` consulta el mundo vivo.

### find_id
Resuelve una instancia viva exacta por id. Si no existe o murió, devuelve `undefined`.

### find_name
Búsqueda global por `name` exacto. Devuelve todas las coincidencias vivas. No filtra implícitamente por parent, group, role o definición. Sin resultados devuelve `[]`.

### find_parent
Devuelve el parent estructural si sigue vivo; de lo contrario `undefined`. Desde `dead(object)` puede usar el objeto muerto de contexto para resolver un parent que continúe vivo.

### find_children
Devuelve sólo hijos Runtime directos, instanciados y vivos. No devuelve declaraciones de spawn, pending, muertos, nietos ni descendientes. Desde `dead(object)` puede usar el objeto muerto de contexto para resolver hijos que sigan vivos.

## Orden y snapshots
`find_name()` y `find_children()` devuelven resultados en orden de entrada al mundo, del más antiguo al más reciente entre los vivos. Los arrays son snapshots JS normales, no colecciones reactivas. Si un elemento muere, el array no se reescribe; esa referencia simplemente deja de ser válida.

## Relación estructural
El parent estructural se establece al originarse la instancia y permanece estable durante su vida. v0.3.0 no expone reparenting. La relación no implica ownership ni propagación automática de lifecycle.

```js
function dead(object) {
    for (const child of find_children(object)) kill(child);
}
```

es una política explícita del juego, no del Runtime.

## Follow
```ts
follow_x(object: RuntimeObject, target: RuntimeObject): void;
follow_y(object: RuntimeObject, target: RuntimeObject): void;
```

`find_*` resuelve; `follow_*` actúa. Follow no conoce nombres, ids ni jerarquía.

## Carry
```ts
carry(object: RuntimeObject, carrier: RuntimeObject): void;
```

Aplica al objeto el delta de movimiento del carrier del frame actual. Es puntual y no crea relación persistente.

## Attach
`attach`, `detach` y `attach_active` conservan su significado: relación viva declarada respecto al parent estructural. Attach no acepta target arbitrario, no cambia parent y no es carry.

## Diagnósticos
Persistir RuntimeObject en local/global debe rechazarse con diagnóstico específico. Reutilizar un wrapper de otro hook debe rechazarse. Usar una referencia normal cuya instancia murió debe rechazarse. FLX no convierte RuntimeObjects a ids ni recupera referencias caducadas automáticamente.

## Microejemplos
- `examples/scripting/references`: identidad y `find_id` / `find_name`.
- `examples/scripting/structural`: `find_parent` / `find_children` y lifecycle estructural explícito.
- `examples/scripting/follow`: resolución + `follow_x`.
- `examples/scripting/carry`: resolución + `carry`.
