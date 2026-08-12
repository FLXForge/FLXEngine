# FLX Timer Characterization

This document characterizes the current Timer contract in FLX.

It does not propose production changes. Its purpose is to record what the
runtime and JavaScript API actually do today before deciding naming,
semantics, or refactors.

Sources reviewed:

- `engine/scripting/bindings/TimerBindings.*`
- `engine/runtime/RuntimeObject.*`
- `engine/runtime/RuntimeWorld.*`
- `docs/scripts/flx.d.ts`
- `docs/en/api.html`
- `docs/es/api.html`
- `docs/scripting-api-characterization.md`
- `docs/scripting-grammar-draft.md`
- `docs/runtime-lifecycle-characterization.md`
- `docs/runtime-object-characterization.md`
- `examples`
- `tests/runtime/RuntimeLifecycleTests.cpp`
- `tests/runtime/RuntimeTimerTests.cpp`

Requested source note:

- `docs/runtime.md` does not exist in the current project tree. Runtime timing
  was contrasted with `docs/runtime-lifecycle-characterization.md` instead.

## A. Current Contract

### Public API

| Function | Signature | Return | Implementation | Documented | d.ts |
| --- | --- | --- | --- | --- | --- |
| `timer` | `timer(object, timerName, duration)` | `undefined` | `TimerBindings.cpp::jsTimer` | yes EN/ES | yes |
| `timer_active` | `timer_active(object, timerName)` | boolean | `TimerBindings.cpp::jsTimerActive` | yes EN/ES | yes |
| `timer_left` | `timer_left(object, timerName)` | number | `TimerBindings.cpp::jsTimerLeft` | yes EN/ES | yes |
| `timer_clear` | `timer_clear(object, timerName)` | `undefined` | `TimerBindings.cpp::jsTimerClear` | yes EN/ES | yes |

No other timer-related JavaScript functions are currently registered.

### `timer(object, name, duration)`

Current behavior:

- Requires at least three arguments. With fewer arguments it returns
  `undefined` and does nothing.
- Resolves `object` by reading `object.id` and calling
  `ScriptEngine::findObjectByRuntimeId`.
- Converts `name` with `JS_ToCString`, then immediately copies it into
  `std::string`.
- Converts `duration` with `JS_ToFloat64`.
- Creates a timer if the name does not exist.
- Starts it immediately by setting `left`.
- If the same name already exists, replaces the remaining time.
- Does not store original duration.
- Does not store a started flag.
- Does not store a finished flag.
- Allows multiple timers per object.
- Timer names are unique inside one `RuntimeObject`.
- Different instances can use the same timer name independently.

The operation currently behaves most like:

```text
create-or-restart timer with remaining time = duration clamped to >= 0
```

Linguistically, the real operation is closer to `start`, `restart`, or `set`
than to a pure noun `timer`. It is not a transition like `state_to`, because it
does not move toward a named state; it replaces a remaining-time value.

No rename is proposed here.

### `timer_active(object, name)`

Current behavior:

- Returns `false` with fewer than two arguments.
- Resolves `object` by `object.id`.
- Converts `name` to `std::string`.
- Returns `true` only when:
  - the timer exists in `object.timers`;
  - `left > 0.0f`.
- Returns `false` for:
  - missing timer;
  - cleared timer;
  - zero-duration timer;
  - negative-duration timer after clamp;
  - finished timer stored at zero.

`timer_active` means "exists and has remaining time", not merely "exists".

### `timer_left(object, name)`

Current behavior:

- Returns `0` with fewer than two arguments.
- Resolves `object` by `object.id`.
- Converts `name` to `std::string`.
- Returns timer `left` when the timer exists.
- Returns `0` when the timer does not exist.
- Finished timers are clamped to `0`.
- Units are seconds.

From JavaScript, `timer_left` cannot distinguish:

- missing timer;
- timer cleared with `timer_clear`;
- timer created with duration `0`;
- timer finished naturally.

### `timer_clear(object, name)`

Current behavior:

- Returns `undefined`.
- With fewer than two arguments it does nothing.
- Resolves `object` by `object.id`.
- Converts `name` to `std::string`.
- Calls `object->timers.erase(name)`.
- If the timer does not exist, nothing happens.
- The name can be reused later with `timer()`.

The name `_clear` accurately describes the current operation: it removes the
entry. It does not set `left` to zero.

## B. Runtime Model

Timers live in `RuntimeObject`:

```cpp
struct RuntimeTimer
{
    float left = 0.0f;
};

std::unordered_map<std::string, RuntimeTimer> timers;
```

Current data per timer:

| Data | Exists | Meaning |
| --- | --- | --- |
| name | yes, as map key | unique timer identity inside one instance |
| original duration | no | cannot be queried or reconstructed reliably |
| left | yes | remaining seconds |
| active flag | no | derived from `left > 0` |
| finished flag | no | not represented |
| just-finished event | no | not represented |
| cleared flag | no | erased entry |

Timers are runtime-only, per-instance state. They do not come from
`ObjectDefinition`, are not copied from JSON, and are not shared between
instances.

When an object is spawned, it starts with an empty `timers` map. Any timers must
be created by script, commonly in `born()`.

When a `RuntimeObject` is destroyed, its timers disappear with the object.

## C. Temporal Semantics

Runtime frame order is currently:

```text
action
spawn
motion
spawn
attachments
collision
spawn
stateTime / timers
dead
cleanup
```

Timers are updated in `RuntimeWorld::updateObjectTime(delta)` after collision
and after spawn flushes, before `dead()` and cleanup.

Current timing rules confirmed by tests:

- A timer created in `born()` keeps its full value until the first update.
- A timer created in `action()` is observable at full value in that same
  callback.
- A timer created in `motion()` is observable at full value in that same
  callback.
- A timer created in `collision()` is observable at full value in that same
  callback.
- Timers created in action, motion, or collision are decremented at the end of
  that same frame.
- The next frame's `action()` observes the value left after the previous
  frame's decrement.
- Decrement uses the safe delta passed to `RuntimeWorld::update`.
- Timers clamp to zero and do not go negative.

The update loop skips timer decrement for objects with `alive == false`.

This matters for `kill()`:

- If an object is killed in `action()`, `updateObjectTime` skips it.
- `dead()` then runs before cleanup.
- During `dead()`, `timer_left()` and `timer_active()` can still query the
  killed object because it still exists in the world until cleanup.
- The dead callback sees the timer value from before the frame decrement.

## D. Edge Cases

Confirmed current behavior:

| Case | Current behavior |
| --- | --- |
| missing timer | `active=false`, `left=0` |
| `duration = 0` | entry is created, `left=0`, `active=false` |
| `duration < 0` | entry is created, `left=0`, `active=false` |
| `duration = NaN` | entry is created, stores `0` today, `active=false` |
| `duration = Infinity` | entry is created, stores infinity today, `active=true` |
| empty name `""` | accepted as a valid key |
| repeated same name | replaces `left` with new clamped duration |
| many names | independent entries in the same object map |
| clear missing name | no error, no effect |
| clear existing name | erases entry |
| restart finished timer | replaces zero with new duration |

No warnings or diagnostics are produced for these edge cases.

There is no validation against `NaN`, `Infinity`, empty names, or extremely many
timers in this layer.

## E. Real Usage

Examples use timers as:

| Example | Files / pattern | Purpose |
| --- | --- | --- |
| Asteroids | `laser.js`, `tail.js`, fragments | lifetime and fade-out |
| Asteroids | asteroid `space.js` | periodic spawn interval |
| Arkanoid | `maintitle/title.js` | short hit animation |
| Arkanoid | `levels/level.js` | intro timing |
| Arkanoid | `paddle.js` | power-up durations and respawn cooldown |
| Arkanoid | `ball.js` | detach cooldown |
| Invaders | `player.js` | shot cooldown |

Observed patterns:

- `timer()` is usually called in `born()` for lifetime/intro timers.
- `timer()` is also called during gameplay to start cooldowns and power-ups.
- `timer_active()` is the dominant query for "can I act now?" or "is effect
  still running?".
- `timer_left()` is used for fade/scaling/interpolation.
- `timer_clear()` is used in Arkanoid to cancel mutually exclusive paddle size
  effects.

No example currently relies on:

- `timer_done`;
- original duration;
- a just-finished event;
- querying whether a finished timer still exists.

## F. Current Grammar

Current timer family:

```text
timer
timer_active
timer_left
timer_clear
```

Grammar observations:

- `timer` is a noun used as an action. This matches the concern already noted
  in `docs/scripting-grammar-draft.md`.
- `timer_active` fits the current `_active` predicate family. It means "running
  with remaining time", not "entry exists".
- `timer_left` is a value projection and is consistent with the draft's
  `CONCEPTO_VALOR` form.
- `timer_clear` is an operation but uses object-then-verb order, like
  `fade_set` and `state_current`, not the `VERBO_COMPLEMENTO` pattern.

The operation performed by `timer()` is not just "create"; it also starts and
restarts. Candidate names should reflect replacement of remaining time.

No naming decision is made here.

## G. Inconsistencies

- The API cannot distinguish "missing", "cleared", "duration zero", and
  "finished"; all report `active=false` and `left=0`.
- Finished timers remain stored internally, but JS has no `timer_exists`.
- There is no `timer_done`, unlike `fade_done`.
- `timer()` has no explicit verb.
- `timer_clear()` is action-like but uses the timer family as prefix.
- Edge values such as `Infinity`, `NaN`, and empty names are accepted silently.
- `timer_active()` could be misread as "timer entry exists", but it actually
  means `left > 0`.

## H. Real Debt

Real technical/design debt observed:

- `RuntimeTimer` only stores `left`; any future `timer_done` or exact
  completion event needs additional state.
- No duration/original duration is stored, limiting later progress ratios unless
  scripts keep constants manually.
- No diagnostics or warnings for invalid duration or invalid name.
- No guard against unbounded timer creation per object.
- The public function `timer()` is semantically overloaded: create, start,
  restart, and set remaining time.
- Timer behavior is runtime-only and not currently represented in structured
  runtime diagnostics.

Not debt:

- Keeping timers per `RuntimeObject` is coherent with the current model.
- Storing timer names as `std::string` is correct; the QuickJS C string is not
  kept after `JS_FreeCString`.
- Finished timers remaining stored at zero is deliberate in current docs and
  tests.

## I. Capabilities That Do Not Exist Yet

The current model does not provide:

- `timer_done(object, name)`;
- `timer_exists(object, name)`;
- `timer_progress(object, name)`;
- original duration;
- elapsed time;
- pause/resume per timer;
- global timers;
- timer callbacks;
- timer groups;
- structured runtime diagnostics for invalid timer calls;
- automatic warning on invalid name/duration;
- one-frame "finished just now" event.

Distinguishing these states would require new runtime data:

```text
A. timer missing
B. timer active
C. timer just finished this frame
D. timer finished previously
E. timer explicitly cleared
```

Today only B is distinguishable through `timer_active()`. A, C, D, and E all
collapse to `active=false` and mostly `left=0` from JavaScript.

## J. Naming Candidates, Without Decision

Given the real operation, plausible future names could include:

| Candidate | Strength | Concern |
| --- | --- | --- |
| `timer_start(object, name, duration)` | clear action; family indexed by timer | still also restarts |
| `start_timer(object, name, duration)` | natural verb-object order | differs from current `timer_*` family |
| `timer_restart(object, name, duration)` | honest for existing timers | odd for first creation |
| `timer_set(object, name, duration)` | matches actual replacement of `left` | `_set` is not preferred in grammar draft |
| `timer_to(object, name, duration)` | aligns with transition pattern | semantically weak: duration is not a destination state |
| keep `timer(...)` | short and already used | noun-as-verb remains unclear |

No definitive recommendation is made in this characterization.

## K. Design Questions

1. Should `timer()` be renamed to a verb form, or is the short noun acceptable
   for v0.3.x?
2. Should timer identity remain a string local to each object?
3. Should empty timer names be invalid?
4. Should non-finite durations be rejected or clamped with warning?
5. Should `timer_active()` continue to mean `left > 0`, or should there also be
   `timer_exists()`?
6. Is `timer_done()` needed, and should it mean "finished ever" or "finished
   this frame"?
7. Should finished timers remain stored at zero, or should they be removed once
   a richer done-state exists?
8. Should timers store original duration to support progress queries?
9. Should timers advance for killed objects before `dead()`, or should the
   current skip-on-dead behavior stay?
10. Should invalid timer operations eventually produce structured runtime
    diagnostics instead of silent no-op / silent clamp?
11. Should timers be able to pause when object state changes, or stay
    independent of state machines?
12. Should there be any limit on timers per object?

## Tests Added

The suite `flx-runtime-timer-tests` characterizes:

- creation;
- restart with same name;
- multiple timers per object;
- `timer_active`;
- `timer_left`;
- `timer_clear`;
- missing timer queries;
- duration `0`;
- negative duration;
- `NaN`;
- `Infinity`;
- empty name;
- creation in `born`, `action`, `motion`, and `collision`;
- exact decrement point after collision;
- finished timers persisting at zero;
- restart after finish;
- interaction with `kill()` and `dead()`;
- interaction with `hide()`;
- interaction with `state_to()`.
