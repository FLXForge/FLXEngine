# RuntimeObject JS

## Estado
- Documento: especificación normativa
- Ámbito: superficie pública JavaScript de RuntimeObject
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Principio

`RuntimeObject` JS es la vista pública de una entidad viva del Runtime.

No es espejo de C++, JSON ni contenedor mutable.

```text
obj observa
funciones Runtime modifican
```

## 2. Superficie pública

```ts
interface RuntimeObject {
    readonly id: string;
    readonly name: string;
    readonly group: string;
    readonly alive: boolean;
    readonly visible: boolean;
    readonly x: number;
    readonly y: number;
    readonly width: number;
    readonly height: number;
    readonly speed: number;
    readonly angle: number;
    readonly velocityX: number;
    readonly velocityY: number;
    readonly rotationSpeed: number;
}
```

`role` se elimina del contrato v0.3: sólo permanecía provisionalmente por Collision y la nueva arquitectura lo sustituye por `group`, named colliders y `CollisionContact`.

`component` tampoco se expone como propiedad genérica JS: es una relación declarativa/runtime consumida por subsistemas. Un componente sigue usando exactamente la misma interfaz RuntimeObject.

## 3. Identidad y vigencia

`id` = identidad concreta de instancia.
`name` = identidad lógica/declarativa.

Wrapper no persistente entre hooks; guardar `id` si se necesita resolver después.

## 4. Estado vivo

Las propiedades públicas son readonly y reflejan cambios Runtime inmediatamente mientras el wrapper sea válido.

Tras `kill(obj)`, `obj.alive` observado dentro del mismo hook pasa inmediatamente a false.

## 5. Mutación

Se realiza mediante operaciones Runtime, por ejemplo:

```ts
position(object,x,y)
position_x(object,x)
position_y(object,y)
position_origin(object)
resize(object,w,h)
resize_width(object,w)
resize_height(object,h)
apply_speed(object,value)
restore_speed(object)
apply_velocity(object,direction,speed)
apply_angle(object,angle)
apply_rotation_speed(object,speed)
show(object)
hide(object)
kill(object)
collider_on(object,name)
collider_off(object,name)
```

No copy-back de propiedades JS.

## 6. Propiedades no públicas

No forman parte del RuntimeObject JS genérico:

```text
role
component
local
controlPlayer
attached
layer
previousX
previousY
originX
originY
originSpeed
```

Que una relación exista internamente no obliga a exponerla como propiedad.

## 7. Scripting state

```ts
type ScriptValue = number | boolean | string;
read_local(object,key): ScriptValue | undefined;
write_local(object,key,value): void;
read_global(key): ScriptValue | undefined;
write_global(key,value): void;
```

RuntimeObject no es ScriptValue ni persistencia.

## 8. Component

`component:true` no crea una interfaz JS distinta.

```text
RuntimeObject normal
RuntimeObject component
→ misma superficie JS
```

State, Lifecycle, Collision y Ray interpretan la relación según sus specs.

## 9. Invariantes

- RO-001 RuntimeObject JS es observacional.
- RO-002 Todas sus propiedades públicas son readonly.
- RO-003 id representa identidad de instancia.
- RO-004 wrapper no es persistencia.
- RO-005 cambios Runtime son observables inmediatamente.
- RO-006 role no forma parte de v0.3.
- RO-007 component no crea tipo JS diferente.
- RO-008 estado correlacional se cambia con operaciones Runtime.
