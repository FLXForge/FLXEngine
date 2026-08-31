# Lifecycle

## Scope

This specification defines the public lifecycle and world-participation contract for runtime instances in FLX v0.3.0.

It covers `spawn()`, `born()`, `kill()`, `dead()`, `show()`, `hide()`, `keep_only()`, and the lifecycle meaning of declarative `children`.

It does not define reference or hierarchy operations such as `attach`, `detach`, `carry`, `follow_x`, or `follow_y`. Those belong to the References / Context / hierarchy audit.

---

## Core lifecycle model

```text
creation request / declarative creation
                |
                v
        enters the world
                |
                v
             born()
            [once]
                |
                v
          +-----------+
          |   ALIVE   |
          +-----------+
                |
              kill()
                |
                v
          alive = false
                |
                v
             dead()
            [once]
                |
                v
             cleanup
                |
                v
        leaves the world
```

The public contract describes these transitions. Internal phase ordering, spawn queues, flush points, or container details are implementation details unless explicitly stated otherwise.

## `spawn()`

```js
spawn(object, childName);
```

`spawn()` requests the creation of a new runtime instance from a child declared in the source object's declarative `children`.

- `spawn()` is a creation request.
- The new instance is incorporated into the world when the Runtime can do so safely.
- The exact internal flush point or frame boundary is not part of the public contract.
- When the instance effectively enters the world, its `born()` hook is executed.
- `spawn()` does not return a newly created `RuntimeObject`.
- A spawn request remains valid even if the source instance disappears before the requested child is incorporated into the world.
- `spawn()` may be called from normal behavior hooks as well as from `born()` and `dead()`.

Scripts must not depend on a guarantee such as “a spawn requested during `action()` is available during `motion()` of the same frame”.

## Declarative `children`

`children` defines the declarative creation context of an object.

### Automatic children

An automatic child is instantiated as part of the creation/composition of its parent instance. Automatic descendants may themselves create their automatic children recursively.

### Manual children

A manual child belongs to the set of instances that its parent is allowed to request through `spawn()`.

### Parent relationship

A runtime child preserves internally the identity of the instance from which it originated. This relationship means origin/context of creation. It does not mean ownership.

Therefore:

- the parent does not own the child's lifecycle;
- killing a parent does not automatically kill its children;
- killing a child does not affect its parent;
- once alive, each child is an independent runtime instance unless another explicit live relationship applies.

Declarative hierarchy determines how an instance may originate. It does not imply shared lifetime.

## `born()`

```js
function born(object) {
}
```

`born()` is the hook associated with the transition into the world.

- `born()` executes once for each runtime instance.
- It runs when the instance effectively enters the world.
- At that point the instance already exists as a valid runtime entity.
- Its effective initial state has already been resolved.
- Declarative creation data, creation inheritance, mechanics state, identity, local scripting state, and creation context are available to the hook.
- `born()` may modify the instance through normal public Runtime operations.
- `born()` may write local or global scripting state.
- `born()` may call `spawn()`.
- `born()` may call `kill()` on the instance itself.

If an instance kills itself during `born()`, the instance is considered to have been born and then immediately transitioned to the dead state. There is no separate “cancelled birth” lifecycle state.

## `kill()`

```js
kill(object);
```

`kill()` performs the transition from alive to dead.

- `kill()` changes the instance's live state immediately.
- After the transition, `object.alive` is `false`.
- The instance is not physically removed at the instant `kill()` is called.
- Once dead, the instance stops participating in normal alive-only Runtime phases.
- The instance remains available long enough for its `dead()` hook to execute.
- The death transition is irreversible for that runtime instance.
- FLX v0.3.0 has no resurrection operation.
- Killing an already dead instance does not create a second death transition and does not cause `dead()` to execute again.

A gameplay “respawn” is represented by creating another runtime instance, not by returning the same runtime identity to `alive = true`.

## `dead()`

```js
function dead(object) {
}
```

`dead()` is the hook associated with the transition out of the alive state.

- `dead()` executes once for each runtime instance that dies.
- It runs after `alive` has become `false`.
- It runs before the Runtime physically removes the instance.
- Inside `dead()`, `object.alive` is `false`.
- The instance still retains enough runtime and declarative context to execute its final behavior.
- `dead()` may call Runtime operations that remain meaningful for a dead-but-not-yet-cleaned-up instance.
- In particular, `dead()` may call `spawn()`.

A spawn request made from `dead()` is not cancelled when the dead source instance is later removed. This allows death behavior such as fragments, explosions, drops, replacement objects, secondary enemies, and other consequences generated by the dying instance.

The exact moment when these requested instances enter the world is controlled by the Runtime and is not part of the public timing contract.

## Unified death path

All normal ways of killing runtime instances converge on the same lifecycle transition:

```text
alive
  |
  v
alive = false
  |
  v
dead()
  |
  v
cleanup
```

There are no separate categories of death depending on which subsystem or public operation caused the transition. A future Runtime operation that kills an instance must preserve the same lifecycle semantics.

## `show()` and `hide()`

```js
show(object);
hide(object);
```

Visibility is exclusively a visual-participation concept.

### `hide()`

`hide()` sets the runtime visibility state to false.

A hidden object:

- remains alive;
- continues normal behavior;
- continues motion and mechanics processing;
- continues collision participation;
- continues state processing;
- continues timer processing;
- continues any other non-render participation that does not explicitly depend on visibility;
- is not rendered.

### `show()`

`show()` sets the runtime visibility state to true. A visible and alive object may participate in rendering again.

### Contract

`visible` must not be interpreted as an `enabled`, `active`, `paused`, or lifecycle property. Hiding an object does not suspend it.

Observation and mutation remain separated:

```js
object.visible
show(object)
hide(object)
```

`object.visible` is readonly public state. Visibility changes are Runtime operations.

## `keep_only()`

```js
keep_only(object);
```

`keep_only()` preserves exactly the specified runtime instance and kills all other currently alive runtime instances present in the world.

- the specified object is kept alive;
- every other currently alive runtime instance is passed through the normal death transition;
- affected instances execute `dead()` once before cleanup;
- parent/child relationships do not grant exemptions;
- children of the kept object are not preserved automatically;
- the parent of the kept object is not preserved automatically;
- declarative hierarchy does not change the meaning of `keep_only()`.

`keep_only()` is not a world freeze and does not guarantee that only one object can exist from that point onward.

In particular:

- already requested spawns are not conceptually invalidated by `keep_only()`;
- `dead()` hooks triggered by `keep_only()` may themselves request new spawns;
- the world continues evolving according to the normal lifecycle rules after the operation.

The operation means: kill all other runtime instances that are alive now. It does not mean: permanently enforce a one-object world.

## World participation summary

```text
alive
    lifecycle participation

visible
    render participation

children
    declarative creation context

spawn()
    request instance creation

kill()
    transition an instance to dead

born()
    react once to entering the world

dead()
    react once to death before cleanup

show()/hide()
    change visual participation only

keep_only()
    kill all other currently alive instances
```

These concepts must remain separate.

Visibility is not lifecycle. Declarative hierarchy is not ownership. Death is not immediate physical deletion. Spawn is not synchronous object construction.

## Public design rules

1. Runtime objects expose observable lifecycle state as readonly information.
2. Changes to lifecycle or world participation occur through explicit Runtime operations.
3. Internal scheduling and safe-processing boundaries are not exposed as scripting guarantees.
4. Parent/child creation context does not imply shared lifetime.
5. Every runtime instance owns its own lifecycle.
6. Every death follows the same one-way transition and executes `dead()` once.
7. `dead()` is a valid place to generate consequences of death through `spawn()`.
8. Visibility controls rendering only.
9. `keep_only()` uses normal death semantics and does not bypass `dead()`.
10. Reference, attachment, carrying, following, and hierarchy navigation are outside this specification.
