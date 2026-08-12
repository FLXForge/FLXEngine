# FLX Timer Characterization

This document records the consolidated Timer contract after the Timer audit.
Timer is now a small runtime process owned by a `RuntimeObject`.

Timer remains:

- runtime-only;
- local to one object instance;
- identified by a string name inside that instance;
- independent from State, visibility and global pause.

`docs/runtime.md` was referenced during the audit, but it does not exist in the
current project tree. Runtime timing was contrasted with
`docs/runtime-lifecycle-characterization.md` instead.

## A. Public API

The public JavaScript Timer API is:

| Function | Signature | Return | Meaning |
| --- | --- | --- | --- |
| `play_timer` | `play_timer(object, name)` | `undefined` | resumes a paused timer or replays a done timer |
| `play_timer` | `play_timer(object, name, duration)` | `undefined` | creates or retimes a timer using a total duration |
| `pause_timer` | `pause_timer(object, name)` | `undefined` | pauses a running timer |
| `stop_timer` | `stop_timer(object, name)` | `undefined` | removes a timer |
| `timer_active` | `timer_active(object, name)` | boolean | true for running or paused timers |
| `timer_paused` | `timer_paused(object, name)` | boolean | true only for paused timers |
| `timer_done` | `timer_done(object, name)` | boolean | true only after natural completion |
| `timer_left` | `timer_left(object, name)` | number | remaining seconds |

Removed public APIs:

- `timer(object, name, duration)`
- `timer_clear(object, name)`

No deprecated aliases are registered.

## B. Runtime Model

Timers live in `RuntimeObject`:

```cpp
enum class RuntimeTimerStatus
{
    Running,
    Paused,
    Done
};

struct RuntimeTimer
{
    float duration = 0.0f;
    float left = 0.0f;
    RuntimeTimerStatus status = RuntimeTimerStatus::Running;
};

std::unordered_map<std::string, RuntimeTimer> timers;
```

The absence of a map entry represents `ABSENT`. There is no stopped status.
Stopping a timer removes it.

## C. Observable States

| State | `timer_active` | `timer_paused` | `timer_done` | `timer_left` |
| --- | --- | --- | --- | --- |
| `ABSENT` | false | false | false | 0 |
| `RUNNING` | true | false | false | `> 0` |
| `PAUSED` | true | true | false | frozen `> 0` |
| `DONE` | false | false | true | 0 |

`active` means the timer is still valid and has not naturally completed or
been stopped. A paused timer is active.

`done` means the timer reached the end naturally. `stop_timer` does not mark a
timer as done.

## D. Validation

Timer names must be strings and must not be empty.

Durations, when provided, must be:

- numbers;
- finite;
- strictly greater than `0`.

Invalid input logs a warning through `Logger`, leaves the timer untouched and
lets script execution continue. This task deliberately does not add structured
Runtime diagnostics for Timer.

## E. `play_timer`

### Without Duration

`play_timer(object, name)` behaves by state:

| State | Result |
| --- | --- |
| `ABSENT` | warning and no-op, because duration is unknown |
| `RUNNING` | no-op |
| `PAUSED` | becomes `RUNNING`, preserving `left` and `duration` |
| `DONE` | restarts from the stored `duration` |

### With Duration

`play_timer(object, name, duration)` uses `duration` as total duration, not as
new remaining time.

| State | Result |
| --- | --- |
| `ABSENT` | creates a running timer with `left = duration` |
| `DONE` | redefines `duration`, sets `left = duration`, runs |
| `RUNNING` / `PAUSED` | preserves elapsed time and changes total duration |

For an existing running or paused timer:

```text
elapsed = oldDuration - left
newLeft = newDuration - elapsed
```

If `newLeft > 0`, the timer becomes running with that remaining time.
If `newLeft <= 0`, the timer becomes done with `left = 0`.

This is intentionally not an automatic restart. A real restart is expressed
explicitly as:

```js
stop_timer(object, "name");
play_timer(object, "name", duration);
```

## F. Pause And Stop

`pause_timer(object, name)`:

- `RUNNING` -> `PAUSED`;
- `PAUSED`, `DONE`, `ABSENT` -> no-op.

It is not a toggle. Resume is done through `play_timer(object, name)`.

`stop_timer(object, name)`:

- removes running, paused or done timers;
- absent timers are a no-op;
- after stop, active, paused and done are all false and left is `0`.

## G. Natural Completion

During update, running timers decrement once per frame. When `left <= 0`:

- `left` is clamped to `0`;
- status becomes `Done`;
- the timer remains stored until `play_timer` or `stop_timer`.

`DONE` is persistent, not a one-frame event.

## H. Frame And Lifecycle Semantics

The consolidated runtime order remains:

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

Rules:

- timers created in `born()` keep full duration until the first update;
- timers created or modified in `action`, `motion` or `collision` keep their
  defined value during that callback;
- running timers decrement at the timer phase of that same frame;
- paused and done timers do not decrement;
- hidden objects still update timers;
- `state_to` does not affect timers;
- killed objects do not advance timers after being killed;
- `dead()` can still query the final timer state before cleanup destroys the
  instance.

## I. Historical Restart Audit

The old `timer(...)` function restarted active timers by replacing `left`.
That behavior is no longer implicit.

Real uses that depended on restart semantics were updated explicitly:

- Asteroids asteroid spawner reset while the game is not running:
  `stop_timer` + `play_timer`.
- Arkanoid paddle respawn and power-up refreshes:
  `stop_timer` + `play_timer`.

No separate `restart_timer` API was added. Current real usage is readable with
the explicit stop/play pair.

## J. Grammar Result

Timer validates `play`, `pause` and `stop` as process-control vocabulary for
capabilities that can run, be suspended and end voluntarily.

Queries remain in the timer family:

- `timer_active`
- `timer_paused`
- `timer_done`
- `timer_left`

This does not mean every future capability must implement all three control
verbs. It only confirms that Timer is a good fit for this vocabulary.

## K. Tests

`flx-runtime-timer-tests` covers:

- creation with duration;
- play without duration on absent timers;
- running retime preserving elapsed;
- duration shorter than elapsed becoming done;
- pause, repeated pause and resume;
- pause plus duration change;
- natural done persistence;
- replaying done timers;
- stop from running, paused, done and absent;
- invalid names and invalid durations;
- multiple timers per instance;
- same timer name on different instances;
- frame timing from `born`, `action`, `motion` and `collision`;
- interaction with `hide`, `state_to`, `kill` and `dead`;
- explicit restart for historical restart use cases.

## L. Open Questions

Deliberately pending:

- structured runtime Diagnostics for invalid Timer calls;
- global game pause policy;
- timer progress/elapsed public queries;
- timer callbacks or events;
- timer groups;
- maximum timers per object.
