// FLX Engine JavaScript API
// Version: 0.3.0
//
// Provides IntelliSense/documentation support for FLX scripts.
//
// Usage:
// /// <reference path="../flx.d.ts" />

/** Logical UP component. Do not depend on its numeric value. */
declare const UP: number;

/** Logical DOWN component. Do not depend on its numeric value. */
declare const DOWN: number;

/** Logical LEFT component. Do not depend on its numeric value. */
declare const LEFT: number;

/** Logical RIGHT component. Do not depend on its numeric value. */
declare const RIGHT: number;

/** Neutral movement constant. */
declare const STOP: number;

/** Logical NEGATIVE component. Do not depend on its numeric value. */
declare const NEGATIVE: number;

/** Logical POSITIVE component. Do not depend on its numeric value. */
declare const POSITIVE: number;

/** Horizontal axis selector for input_direction(). */
declare const HORIZONTAL: number;

/** Vertical axis selector for input_direction(). */
declare const VERTICAL: number;

type ScriptValue = number | boolean | string;

/**
 * Runtime representation of an object created by FLX.
 *
 * RuntimeObject is a temporary live view for the current script hook.
 * Do not store it in local/global state. Store its id string and resolve it
 * again with find_id() when needed.
 * A normal reference expires when the current hook ends or when its instance
 * dies. dead(object) deliberately receives the final context of the dead
 * instance for that hook.
 */
interface RuntimeObject {
    /** Unique runtime instance identifier. */
    readonly id: string;
    
    /** Logical instance name assigned by the parent children map. */
    readonly name: string;

    /** Collision group identifier. */
    readonly group: string;

    /** True while the object is alive. Read-only from JavaScript. Use kill(object). */
    readonly alive: boolean;

    /** True while the object can produce visual output. Read-only from JavaScript. Use show(object) and hide(object). */
    readonly visible: boolean;

    /** Current X position. */
    readonly x: number;

    /** Current Y position. */
    readonly y: number;

    /** Runtime object width. */
    readonly width: number;

    /** Runtime object height. */
    readonly height: number;

    /** Current movement speed. Used by classic speed + angle movement. */
    readonly speed: number;

    /**
     * Current object rotation in degrees.
     *
     * FLX convention:
     * 0 = up
     * 90 = right
     * 180 = down
     * 270 = left
     */
    readonly angle: number;

    /** Current horizontal velocity. Used by motion-based movement. */
    readonly velocityX: number;

    /** Current vertical velocity. Used by motion-based movement. */
    readonly velocityY: number;

    /** Runtime rotation speed in degrees per second. */
    readonly rotationSpeed: number;

    /**
     * Visual draw depth.
     * Lower values are drawn first. Mutate it with depth(object, value).
     * Changes affect the next draw pass order.
     */
    readonly depth: number;
}

/**
 * Result returned by ray().
 */
interface CollisionContact {
    readonly collider: string;
    readonly otherCollider: string;
    readonly normalX: number;
    readonly normalY: number;
    readonly pointX: number;
    readonly pointY: number;
    readonly penetration: number;
}

interface RayHit {
    readonly object: RuntimeObject;
    readonly collider: string;
    readonly pointX: number;
    readonly pointY: number;
    readonly normalX: number;
    readonly normalY: number;
    readonly distance: number;
}

interface InputButton {
    readonly __flxInputControl?: "button";
    readonly index: number;
}

interface InputDirection {
    readonly __flxInputControl?: "direction";
    readonly index: number;
}

interface InputPlayerSubject {
    readonly __flxInputSubject?: "player";
    readonly index: number;
}

interface InputSystemSubject {
    readonly __flxInputSubject?: "system";
    readonly index: number;
}

type InputButtonSubject =
    RuntimeObject |
    InputPlayerSubject |
    InputSystemSubject;
type InputDirectionSubject =
    RuntimeObject |
    InputPlayerSubject;

/** Returns a logical button descriptor. */
declare function button(index: number): InputButton;

/** Returns a logical direction descriptor. */
declare function direction(index: number): InputDirection;

/** Returns an explicit player input subject. Player indexes start at 1. */
declare function player(playerIndex: number): InputPlayerSubject;

/** Returns the system input subject. */
declare function system(): InputSystemSubject;

/** True on the frame a mapped button or direction component becomes active. */
declare function input_pressed(subject: InputButtonSubject, control: InputButton): boolean;
declare function input_pressed(subject: InputDirectionSubject, control: InputDirection, component: number): boolean;

/** True while a mapped button or direction component is active. */
declare function input_down(subject: InputButtonSubject, control: InputButton): boolean;
declare function input_down(subject: InputDirectionSubject, control: InputDirection, component: number): boolean;

/** True on the frame a mapped button or direction component stops being active. */
declare function input_released(subject: InputButtonSubject, control: InputButton): boolean;
declare function input_released(subject: InputDirectionSubject, control: InputDirection, component: number): boolean;

/**
 * Returns -1, 0 or 1 for a logical direction on a concrete axis.
 * RIGHT and DOWN are positive; LEFT and UP are negative.
 * The axis selector must be HORIZONTAL or VERTICAL.
 * In 4way, HORIZONTAL reads LEFT/RIGHT and VERTICAL reads UP/DOWN.
 * In 2way, POSITIVE/NEGATIVE project onto either axis.
 */
declare function input_direction(subject: InputDirectionSubject, control: InputDirection, axis: number): number;

/** Reads a value from the local state owned by one runtime object. */
declare function read_local(object: RuntimeObject, key: string): ScriptValue | undefined;

/** Writes a number, boolean or string into the local state owned by one runtime object. */
declare function write_local(object: RuntimeObject, key: string, value: ScriptValue): void;

/** Reads a value from the shared runtime state. */
declare function read_global(key: string): ScriptValue | undefined;

/** Writes a number, boolean or string into the shared runtime state. */
declare function write_global(key: string, value: ScriptValue): void;

/**
 * Resolves a live runtime object by runtime id during the current hook.
 * Returns undefined when the object does not exist or is not alive.
 */
declare function find_id(id: string): RuntimeObject | undefined;

/**
 * Returns all live runtime objects with the given logical instance name, in world order.
 * The returned array is a normal JavaScript snapshot, not a reactive collection.
 */
declare function find_name(name: string): RuntimeObject[];

/**
 * Returns the live parent of an object, when it still exists.
 * In dead(object), the dead object may be used as final structural context,
 * but the returned parent must still be alive.
 */
declare function find_parent(object: RuntimeObject): RuntimeObject | undefined;

/**
 * Returns the live direct children of an object, in world order.
 * It does not return declarations, pending objects, grandchildren or dead
 * children. The returned array is a normal JavaScript snapshot.
 */
declare function find_children(object: RuntimeObject): RuntimeObject[];

/**
 * Moves an object on the horizontal axis using an intent from -1 to 1.
 */
declare function move_horizontal(object: RuntimeObject, intent: number): void;

/**
 * Moves an object on the vertical axis using an intent from -1 to 1.
 */
declare function move_vertical(object: RuntimeObject, intent: number): void;

/**
 * Moves an object using its current mechanics state.
 */
declare function advance(object: RuntimeObject): void;

/**
 * Rotates an object using the declared rotation mechanics or runtime rotationSpeed.
 *
 * @example
 * rotate(object, -1);
 * rotate(object, 1);
 */
declare function rotate(object: RuntimeObject, intent?: number): void;

/**
 * Makes an object follow another object on the X axis.
 * The target must be a live RuntimeObject, not a name.
 */
declare function follow_x(object: RuntimeObject, target: RuntimeObject): void;

/**
 * Makes an object follow another object on the Y axis.
 * The target must be a live RuntimeObject, not a name.
 */
declare function follow_y(object: RuntimeObject, target: RuntimeObject): void;

/**
 * Enables declared attach rules for an object and its original parent.
 * Captures the object's current relative position as the new live attach offset
 * for the followed axes.
 */
declare function attach(object: RuntimeObject): void;

/**
 * Disables attach rules. The object keeps its current position and angle, and
 * the original creation offset remains unchanged.
 */
declare function detach(object: RuntimeObject): void;

/**
 * Returns whether an object is currently attached.
 */
declare function attach_active(object: RuntimeObject): boolean;

/**
 * Applies the carrier movement delta to an object for the current frame.
 * Does not create a persistent relationship.
 */
declare function carry(object: RuntimeObject, carrier: RuntimeObject): void;

/**
 * Reflects an object across the X axis of its movement.
 */
declare function reflect_x(object: RuntimeObject): void;

/**
 * Reflects an object across the Y axis of its movement.
 */
declare function reflect_y(object: RuntimeObject): void;

/**
 * Accelerates the object using declared mechanics and moves it this frame.
 */
declare function accelerate(object: RuntimeObject, intent?: number): void;

/**
 * Applies a runtime movement speed immediately.
 */
declare function apply_speed(object: RuntimeObject, value: number): void;

/** Applies a runtime angle immediately. */
declare function apply_angle(object: RuntimeObject, angle: number): void;

/** Applies a runtime rotation speed immediately. */
declare function apply_rotation_speed(object: RuntimeObject, speed: number): void;

/**
 * Replaces the live linear velocity vector using a direction and magnitude.
 * Does not change angle, rotation, declared speed, acceleration or inertia.
 */
declare function apply_velocity(
    object: RuntimeObject,
    direction: number,
    speed: number
): void;

/**
 * Restores the runtime movement speed declared at creation.
 */
declare function restore_speed(object: RuntimeObject): void;

/**
 * Places an object at the given logical coordinates.
 */
declare function position(object: RuntimeObject, x: number, y: number): void;

/**
 * Applies a directed collision contact correction to the object's current
 * position: position += contact.normal * contact.penetration.
 *
 * This does not reflect velocity, change angle, change speed, or perform
 * automatic physics resolution.
 */
declare function position(object: RuntimeObject, contact: CollisionContact): void;

/** Places an object on the X axis. */
declare function position_x(object: RuntimeObject, x: number): void;

/** Places an object on the Y axis. */
declare function position_y(object: RuntimeObject, y: number): void;

/** Resizes an object. */
declare function resize(object: RuntimeObject, width: number, height: number): void;

/** Changes only the runtime object width. */
declare function resize_width(object: RuntimeObject, width: number): void;

/** Changes only the runtime object height. */
declare function resize_height(object: RuntimeObject, height: number): void;

/** Places an object at its original logical coordinates. */
declare function position_origin(object: RuntimeObject): void;

/**
 * Changes the object's draw depth.
 * The current JavaScript view observes the new value immediately, while the
 * current draw pass keeps the order snapshot taken at its start.
 */
declare function depth(object: RuntimeObject, value: number): void;

/**
 * Overrides the object's runtime visual color.
 */
declare function apply_color(object: RuntimeObject, color: string): void;

/**
 * Restores the visual color declared when the object was created.
 */
declare function restore_color(object: RuntimeObject): void;

/**
 * Returns true according to a probability chance.
 *
 * By default, base is 100, so probability(40) means 40%.
 *
 * @example
 * if (probability(40)) {
 *     console.log("Triggered");
 * }
 *
 * @example
 * if (probability(1, 6)) {
 *     console.log("One chance in six");
 * }
 */
declare function probability(chance: number, base?: number): boolean;

/**
 * Returns a random number between min and max.
 *
 * @example
 * apply_angle(asteroid, random(0, 360));
 */
declare function random(min: number, max: number): number;

/**
 * Casts an invisible ray from an object and returns the nearest effective collider.
 */
declare function ray(
    source: RuntimeObject,
    angle: number,
    distance: number
): RayHit | undefined;

/** Enables a declared collider. */
declare function collider_on(object: RuntimeObject, colliderName: string): void;

/** Disables a declared collider. */
declare function collider_off(object: RuntimeObject, colliderName: string): void;

/**
 * Marks an object for destruction.
 * The object will be removed at the end of the frame.
 */
declare function kill(object: RuntimeObject): void;

/**
 * Enables visual output for an object.
 */
declare function show(object: RuntimeObject): void;

/**
 * Disables visual output for an object.
 *
 * Hidden objects still run action, motion, collision, state time and timers.
 */
declare function hide(object: RuntimeObject): void;

/**
 * Marks every living runtime object for destruction except the exact object passed.
 * Children are not preserved automatically.
 */
declare function keep_only(object: RuntimeObject): void;

/**
 * Returns elapsed time in seconds since previous frame.
 * The value is clamped by FLX to avoid abnormal frame spikes.
 */
declare function delta(): number;

/**
 * Creates a child declared in the object's children map.
 *
 * Children with spawn = "auto" are created when the parent enters the world,
 * but they may also be created later with spawn() if another instance is needed.
 *
 * @example
 * spawn(object, "laser");
 */
declare function spawn(
    object: RuntimeObject,
    childName: string
): void;

/**
 * Returns true when the object's iterator creation is still active.
 *
 * Individual and grid creation return false. A finite iterator remains active
 * while its pattern still has pending entries or while its produced instances
 * are alive. A repeating iterator remains active while the owner exists.
 */
declare function creation_active(object: RuntimeObject): boolean;

/**
 * Requests a transition to another state using the object's JSON states
 * declaration.
 */
declare function state_to(object: RuntimeObject, stateName: string): void;

/**
 * Returns the current state name, or an empty string if the object has none.
 */
declare function state_current(object: RuntimeObject): string;

/**
 * Returns true when the object has a state machine and is currently in the
 * given state.
 */
declare function state_active(
    object: RuntimeObject,
    stateName: string
): boolean;

/**
 * Returns true only during the first full runtime frame after the object
 * enters its current state.
 */
declare function state_entered(object: RuntimeObject): boolean;

/**
 * Returns seconds elapsed since the object entered its current state.
 * The first full frame of a state reports 0.
 */
declare function state_time(object: RuntimeObject): number;

/**
 * Starts, resumes, or redefines a named timer owned by the object.
 *
 * Without duration, play_timer resumes a paused timer or replays a done timer
 * from its known duration. With duration, the value represents total duration,
 * not remaining time.
 */
declare function play_timer(
    object: RuntimeObject,
    timerName: string,
    duration?: number
): void;

/**
 * Pauses a running timer without clearing its remaining time.
 */
declare function pause_timer(
    object: RuntimeObject,
    timerName: string
): void;

/**
 * Stops and removes a timer. Stop does not mark the timer as done.
 */
declare function stop_timer(
    object: RuntimeObject,
    timerName: string
): void;

/**
 * Returns true while the named timer is running or paused.
 */
declare function timer_active(
    object: RuntimeObject,
    timerName: string
): boolean;

/**
 * Returns true while the named timer is paused.
 */
declare function timer_paused(
    object: RuntimeObject,
    timerName: string
): boolean;

/**
 * Returns true when the named timer reached the end naturally.
 */
declare function timer_done(
    object: RuntimeObject,
    timerName: string
): boolean;

/**
 * Returns remaining seconds for the named object timer, or 0 when absent,
 * stopped, or done.
 */
declare function timer_left(
    object: RuntimeObject,
    timerName: string
): number;

/**
 * Draws immediate text during the current draw(object) hook.
 *
 * Coordinates are expressed in FLX logical world space and represent the text
 * center/pivot. When a RuntimeObject is passed first, coordinates are local to
 * that reference object and rotate with it. The primitive belongs to the
 * current draw owner, not necessarily to the coordinate reference.
 *
 * @example
 * draw_text(10, 10, "SCORE: " + read_global("score"));
 *
 * @example
 * draw_text(10, 25, "LIVES: " + read_global("lives"), 8);
 *
 * @example
 * draw_text(10, 40, "READY", 10, "#ffffff");
 */
declare function draw_text(
    x: number,
    y: number,
    text: string,
    size?: number,
    color?: string
): void;
declare function draw_text(
    reference: RuntimeObject,
    x: number,
    y: number,
    text: string,
    size?: number,
    color?: string
): void;

/**
 * Draws one immediate logical pixel during the current draw(object) hook.
 * With a RuntimeObject first argument, coordinates are local to that reference.
 */
declare function draw_pixel(
    x: number,
    y: number,
    color?: string
): void;
declare function draw_pixel(
    reference: RuntimeObject,
    x: number,
    y: number,
    color?: string
): void;

/**
 * Draws an immediate line during the current draw(object) hook.
 * With a RuntimeObject first argument, endpoints are local to that reference.
 */
declare function draw_line(
    x: number,
    y: number,
    x1: number,
    y1: number,
    color?: string
): void;
declare function draw_line(
    reference: RuntimeObject,
    x: number,
    y: number,
    x1: number,
    y1: number,
    color?: string
): void;

/**
 * Draws an immediate rectangle outline during the current draw(object) hook.
 * x/y are the rectangle center/pivot. With a RuntimeObject first argument, the
 * center is local to that reference and the rectangle rotates with it.
 */
declare function draw_rectangle(
    x: number,
    y: number,
    width: number,
    height: number,
    color?: string
): void;
declare function draw_rectangle(
    reference: RuntimeObject,
    x: number,
    y: number,
    width: number,
    height: number,
    color?: string
): void;

/**
 * Starts a full-screen fade from transparent to opaque.
 *
 * Default color is black and duration is currently fixed to one second.
 *
 * @example
 * fade_on();
 *
 * @example
 * fade_on("#000000");
 */
declare function fade_on(color?: string): void;

/**
 * Starts a full-screen fade from opaque to transparent.
 *
 * Default color is black and duration is currently fixed to one second.
 *
 * @example
 * fade_off();
 *
 * @example
 * fade_off("black");
 */
declare function fade_off(color?: string): void;

/**
 * Sets the fade overlay alpha immediately.
 *
 * Alpha is clamped between 0 and 1.
 *
 * @example
 * fade_set(1);
 * fade_off();
 *
 * @example
 * fade_set(0, "black");
 */
declare function fade_set(
    alpha: number,
    color?: string
): void;

/**
 * Returns true while a fade transition is running.
 */
declare function fade_active(): boolean;

/**
 * Returns true when there is no fade transition running.
 */
declare function fade_done(): boolean;

/**
 * Returns the current fade alpha.
 * 0 means fully visible; 1 means fully covered.
 */
declare function fade_alpha(): number;

/**
 * Plays a sound declared in the object's sounds map.
 *
 * @example
 * play_sound(ship, "laser");
 */
declare function play_sound(
    object: RuntimeObject,
    id: string
): void;

/**
 * Plays generated music declared in the object's music map.
 * Replaces the currently playing music, if any.
 *
 * @example
 * play_music(game, "theme");
 */
declare function play_music(
    object: RuntimeObject,
    id: string
): void;

/**
 * Stops the current generated music.
 */
declare function stop_music(): void;

/**
 * Pauses the current music, or resumes it if it is already paused.
 */
declare function pause_music(): void;

/**
 * Returns true while generated music is currently active.
 */
declare function music_active(): boolean;

/**
 * Returns true when the current music is paused.
 */
declare function music_paused(): boolean;

/**
 * Requests an orderly shutdown of the current FLX runtime.
 */
declare function exit(): void;

/**
 * Saves a boolean, number or string value in saves/<name>.flxsave.
 */
declare function save(
    name: string,
    key: string,
    value: boolean | number | string
): void;

/**
 * Loads a boolean, number or string value from saves/<name>.flxsave.
 * Returns defaultValue when the file, key or expected type is unavailable.
 */
declare function load<T extends boolean | number | string>(
    name: string,
    key: string,
    defaultValue: T
): T;

/**
 * Called when an object enters the world.
 * Executed for auto children and manually spawned children.
 */
declare function born(object: RuntimeObject): void;

/**
 * Optional action phase function.
 */
declare function action(object: RuntimeObject): void;

/**
 * Optional motion phase function.
 */
declare function motion(object: RuntimeObject): void;

/**
 * Called when one or more source colliders declared in this object contact
 * another object whose group is listed in those colliders' with arrays.
 */
declare function collision(
    object: RuntimeObject,
    other: RuntimeObject,
    contacts: CollisionContact[]
): void;

/**
 * Optional draw phase function.
 */
declare function draw(object: RuntimeObject): void;

/**
 * Called once before an object is removed.
 * Executed after kill() and before cleanup.
 * The object is dead, but may still be used as final context for operations
 * such as spawn(), read_local(), play_sound(), find_parent() and find_children().
 */
declare function dead(object: RuntimeObject): void;
