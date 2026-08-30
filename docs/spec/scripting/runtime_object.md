# RuntimeObject JS

## Purpose

`RuntimeObject` is the public JavaScript view of a living FLX Runtime entity.

It is not a mirror of the C++ `RuntimeObject`, a representation of the source JSON, or a mutable scripting container.

The JavaScript object answers two questions:

- what runtime entity is this?
- what is its current public state?

The object is observational. Changes to the object or to the world are performed through Runtime API functions.

## Core contract

A JavaScript `RuntimeObject` is a readonly live view of the Runtime entity delivered to the current hook.

Its public properties resolve the current Runtime state while that view is valid. Therefore, a Runtime operation performed during a hook must be immediately observable through subsequent reads of the same JavaScript object.

For example, after changing an object's X position with `position_x()`, reading `obj.x` in the same hook returns the new Runtime value.

There is no JavaScript-to-Runtime property copy-back step.

Direct assignment to public `RuntimeObject` properties is not part of the API.

## Public surface

```ts
interface RuntimeObject {
    readonly id: string;
    readonly name: string;

    readonly group: string;
    readonly role: string;

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

`group` and `role` remain part of the public surface in v0.3.0 because they are currently used by Collision behavior. Their final placement and semantics remain subject to the Collision audit.

## Identity

### `name`

`name` is the logical or declarative identity of the object.

It identifies what declared instance or logical element the behavior is acting on.

### `id`

`id` is the unique identity of the concrete Runtime instance.

It is the value intended to represent instance identity beyond the immediate JavaScript view.

The API does not guarantee that a retained JavaScript `RuntimeObject` reference remains valid between hooks. Code that needs to preserve identity beyond the current hook should preserve `id`, not assume that the JavaScript wrapper itself is persistent.

No public lookup API such as `find()` is defined by this specification.

## Live state

The following properties expose current Runtime state:

```text
alive
visible
x
y
width
height
speed
angle
velocityX
velocityY
rotationSpeed
```

They are readonly from JavaScript.

A property is exposed because behavior has a legitimate need to observe it, not merely because an equivalent field exists internally in C++.

Internal Runtime state is not automatically part of the scripting contract.

## Mutation model

`RuntimeObject` does not modify the Runtime through property assignment.

Runtime changes are expressed through functions.

This establishes the general rule:

> `obj` observes; Runtime functions modify.

When a writable property is removed from `RuntimeObject`, FLX must retain equivalent expressive capability through a public Runtime operation where that capability is legitimate.

### Positioning

```ts
position(object, x, y): void;
position_x(object, x): void;
position_y(object, y): void;
position_origin(object): void;
```

`position()` places the object at an absolute logical position.

`position_x()` changes only the live X coordinate.

`position_y()` changes only the live Y coordinate.

These operations are not guided movement and are not equivalent to `move_horizontal()` or `move_vertical()`.

`position_origin()` restores the declared origin position.

No `position_origin_x()` or `position_origin_y()` operations are defined in v0.3.0.

### Live Mechanics state

```ts
apply_speed(object, value): void;
apply_velocity(object, direction, speed): void;
apply_angle(object, angle): void;
apply_rotation_speed(object, speed): void;
restore_speed(object): void;
```

`apply_angle()` replaces the current absolute live angle.

It is distinct from `rotate()`, which performs a rotation operation according to Mechanics.

`apply_rotation_speed()` replaces the current live rotation speed used by rotation behavior. It does not rotate the object immediately.

`apply_speed()` and `apply_velocity()` retain their Mechanics contracts.

`restore_speed()` restores the effective declared starting speed.

### Size

Size uses its own operation family:

```ts
resize(object, width, height): void;
resize_width(object, width): void;
resize_height(object, height): void;
```

`resize()` changes both live dimensions.

`resize_width()` changes only width.

`resize_height()` changes only height.

## Removed RuntimeObject properties

The following values are not part of the public JavaScript `RuntimeObject` contract:

```text
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

Their removal from JavaScript does not imply that equivalent internal state cannot exist in Runtime subsystems.

### `attached`

Attachment state belongs to world/runtime operations and is observed or modified through the corresponding attachment API rather than through a writable object property.

### `layer`

The internal render layer is not exposed as generic object state. Render ordering and future layer concepts belong to the rendering-related audits and must not be conflated with the current internal field.

### Previous and origin values

`previousX`, `previousY`, `originX`, `originY`, and `originSpeed` are internal or reference state whose direct observation is not currently justified by behavior.

Operations such as `position_origin()` and `restore_speed()` expose the useful behavior without exposing the underlying stored values.

### `controlPlayer`

Input ownership/configuration is abstracted by the Input API. Behavior uses Runtime objects or explicit subjects such as `player(n)` rather than reading the object's configured player index directly.

## Scripting state

Mutable behavior state is separate from `RuntimeObject`.

FLX defines two scripting state scopes:

- local state, associated with one Runtime instance;
- global state, shared by the current Runtime/game execution.

The supported scripting value types in v0.3.0 are:

```ts
type ScriptValue = number | boolean | string;
```

Objects, arrays, RuntimeObject references and other compound values are not part of the v0.3.0 scripting state contract.

JavaScript uses `number` for both integer and decimal numeric values.

## Local state

Local state belongs to a concrete Runtime instance.

It is accessed through:

```ts
read_local(object, key): ScriptValue | undefined;
write_local(object, key, value): void;
```

Each Runtime instance has independent local state.

Writing local state affects only that instance.

When the Runtime instance disappears, its local state disappears with it.

Local state is not persistence and is unrelated to save files.

### Missing keys

Reading a key that does not exist returns JavaScript `undefined`.

`0`, `false` and `""` are valid stored values and must therefore remain distinguishable from an absent key.

No default-value overload is defined in v0.3.0.

No dedicated `clear`, `reset`, `remove`, `increment` or equivalent local-state operations are defined in v0.3.0.

## Declarative local state

An object declaration may define its initial local scripting state:

```json
{
    "local": {
        "lives": 3,
        "factor": 0.75,
        "active": true,
        "phase": "attack"
    }
}
```

This data initializes the local state of each Runtime instance created from the effective declaration.

Each instance receives independent live local state.

Changes made through `write_local()` do not modify the declaration and do not affect other instances.

The existence of declarative `local` does not expose `obj.local` in JavaScript.

## Global state

Global scripting state is shared by behaviors within the current Runtime/game execution.

It is accessed through:

```ts
read_global(key): ScriptValue | undefined;
write_global(key, value): void;
```

A write is immediately visible to later reads from any behavior participating in the same Runtime execution.

Reading a missing key returns `undefined`.

Global scripting state is not persistence.

There is no public mutable JavaScript `global` object.

## No declarative global JSON block

FLX v0.3.0 does not define a JSON block named `global` for scripting state.

No individual FLX object JSON document is conceptually global, so scripting scope must not be imposed on the document model merely for symmetry with `local`.

Global scripting state may be initialized explicitly from behavior using `write_global()`.

This specification does not define a future declarative source for global state.

## Lifetime and hook validity

The Runtime provides a JavaScript `RuntimeObject` view when invoking a behavior hook.

Within that hook, the view resolves current Runtime state and reflects changes made by Runtime operations immediately.

The JavaScript API does not guarantee persistence of that wrapper between hook executions.

This keeps JavaScript object lifetime separate from Runtime instance identity.

`id` is the public instance identity value.

## Death and `alive`

An object whose `alive` state becomes false stops participating in normal Runtime behavior phases according to the Runtime lifecycle.

The Runtime may still execute the corresponding `dead()` lifecycle hook before cleanup removes the instance.

If a hook performs:

```js
kill(obj);
```

then a subsequent read of:

```js
obj.alive
```

within that same valid hook view returns `false`.

No persistent stale-reference contract is defined after the Runtime instance has been cleaned up.

## Architectural boundary

The public JavaScript object is defined from the needs of behavior, not from the structure of C++ or JSON.

The intended separation is:

```text
RuntimeObject JS
    identifies and observes a Runtime entity

Runtime API functions
    modify the entity or the world

Scripting state API
    stores mutable behavior state
```

This boundary is part of the v0.3.0 scripting contract.
