// FLX Engine JavaScript API
// Version: 0.0.1
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

type AudioSourceType = "oscillator" | "noise" | "impact" | "pulse";
type AudioWave = "sine" | "square" | "triangle" | "saw" | "pulse" | "noise";
type AudioMovementType = "none" | "rise" | "fall" | "pulse" | "wobble" | "scatter" | "random";
type AudioSpaceMode = "mono" | "stereo";
type MusicLength = "1/1" | "1/2" | "1/4" | "1/8" | "1/16";

interface AudioSourceConfig {
    type?: AudioSourceType;
    wave?: AudioWave;
    /** Pulse duty cycle from 0.05 to 0.95. Only pulse uses it; square is fixed at 0.5. */
    duty?: number;
}

interface AudioMovementConfig {
    type?: AudioMovementType;
    amount?: number;
}

type AudioNoteConfig =
    string |
    number |
    {
        frequency: number;
    };

interface AudioToneConfig {
    material?: {
        brightness?: number;
        roughness?: number;
        noise?: number;
        resonance?: number;
        metal?: number;
    };
    envelope?: {
        attack?: number;
        decay?: number;
        sustain?: number;
        release?: number;
    };
    space?: {
        mode?: AudioSpaceMode;
        width?: number;
        echo?: number;
    };
}

interface SoundConfig {
    kind?: {
        source?: AudioSourceConfig | string;
        note?: AudioNoteConfig;
        slide?: number;
        movement?: AudioMovementConfig | string;
    };
    tone?: AudioToneConfig | string;
    duration?: number;
    volume?: number;
}

interface InstrumentConfig {
    source?: AudioSourceConfig | string;
    tone?: AudioToneConfig | string;
    play?: {
        legato?: boolean;
        glide?: number;
        vibrato?: number;
    };
    range?: {
        min?: string;
        max?: string;
    };
}

interface MusicChannelConfig {
    instrument?: InstrumentConfig | string;
    wave?: AudioWave;
    volume?: number;
    length?: MusicLength;
    notes: string[];
}

/**
 * Runtime representation of an object created by FLX.
 */
interface RuntimeObject {
    /** Unique runtime instance identifier. */
    id: string;
    
    /** Logical instance name assigned by the parent children map. */
    name: string;

    /**
     * Runtime local state for this object.
     * Values persist while the object exists.
     */
    local: Record<string, number>;

    /** Collision group identifier. */
    group: string;

    /** Optional role inside the collision group. */
    role: string;

    /** True while the object is alive. Read-only from JavaScript. Use kill(object). */
    readonly alive: boolean;

    /** True while the object can produce visual output. Read-only from JavaScript. Use show(object) and hide(object). */
    readonly visible: boolean;

    /** Draw layer. Lower values are drawn first. */
    layer: number;

    /** True while the object is attached to its original parent. */
    attached: boolean;

    /**
     * Player index declared by control.player in JSON.
     * A value of 0 means this object is not bound to a player.
     */
    readonly controlPlayer: number;

    /** Current X position. */
    x: number;

    /** X position at the beginning of the current frame. */
    previousX: number;

    /** Current Y position. */
    y: number;

    /** Y position at the beginning of the current frame. */
    previousY: number;

    /** Runtime object width. */
    width: number;

    /** Runtime object height. */
    height: number;

    /** Current movement speed. Used by classic speed + angle movement. */
    speed: number;

    /** Initial movement speed. */
    originSpeed: number;

    /**
     * Current object rotation in degrees.
     *
     * FLX convention:
     * 0 = up
     * 90 = right
     * 180 = down
     * 270 = left
     */
    angle: number;

    /** Current horizontal velocity. Used by motion-based movement. */
    velocityX: number;

    /** Current vertical velocity. Used by motion-based movement. */
    velocityY: number;

    /** Runtime rotation speed in degrees per second. */
    rotationSpeed: number;

    /** Initial X position. */
    originX: number;

    /** Initial Y position. */
    originY: number;

}

/**
 * Result returned by ray().
 */
interface RayResult {
    /** True when the ray hit a compatible collision object. */
    hit: boolean;

    /** Collision group hit by the ray, or empty string when hit is false. */
    group: string;

    /** Distance from the ray origin to the impact point. */
    distance?: number;

    /** Impact X coordinate in logical FLX space. */
    x?: number;

    /** Impact Y coordinate in logical FLX space. */
    y?: number;
}

/**
 * Shared numeric game state available to all scripts.
 *
 * Unlike object.local, global is shared across the whole game.
 *
 * @example
 * global["score"] = 0;
 * global["lives"] = 3;
 */
declare const global: Record<string, number>;

interface InputButton {
    readonly __flxInputControl?: "button";
    readonly index: number;
}

interface InputDirection {
    readonly __flxInputControl?: "direction";
    readonly index: number;
}

interface InputSubject {
    readonly __flxInputSubject?: "player" | "system";
    readonly index: number;
}

type InputControl = InputButton | InputDirection;
type InputReadableSubject = RuntimeObject | InputSubject;

/** Returns a logical button descriptor. */
declare function button(index: number): InputButton;

/** Returns a logical direction descriptor. */
declare function direction(index: number): InputDirection;

/** Returns an explicit player input subject. Player indexes start at 1. */
declare function player(playerIndex: number): InputSubject;

/** Returns the system input subject. */
declare function system(): InputSubject;

/** True on the frame a mapped button or direction component becomes active. */
declare function input_pressed(subject: InputReadableSubject, control: InputButton): boolean;
declare function input_pressed(subject: InputReadableSubject, control: InputDirection, component: number): boolean;

/** True while a mapped button or direction component is active. */
declare function input_down(subject: InputReadableSubject, control: InputButton): boolean;
declare function input_down(subject: InputReadableSubject, control: InputDirection, component: number): boolean;

/** True on the frame a mapped button or direction component stops being active. */
declare function input_released(subject: InputReadableSubject, control: InputButton): boolean;
declare function input_released(subject: InputReadableSubject, control: InputDirection, component: number): boolean;

/**
 * Returns -1, 0 or 1 for a logical direction on a concrete axis.
 */
declare function input_direction(subject: InputReadableSubject, control: InputDirection, axis: number): number;

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
 */
declare function follow_x(object: RuntimeObject, targetName: string): void;

/**
 * Makes an object follow another object on the Y axis.
 */
declare function follow_y(object: RuntimeObject, targetName: string): void;

/**
 * Enables declared attach rules for an object and its original parent.
 */
declare function attach(object: RuntimeObject): void;

/**
 * Disables attach rules. The object keeps its current position and angle.
 */
declare function detach(object: RuntimeObject): void;

/**
 * Returns whether an object is currently attached.
 */
declare function attach_active(object: RuntimeObject): boolean;

/**
 * Applies the carrier movement delta to an object for the current frame.
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

/**
 * Restores the runtime movement speed declared at creation.
 */
declare function restore_speed(object: RuntimeObject): void;

/** Places an object at the given logical coordinates. */
declare function position(object: RuntimeObject, x: number, y: number): void;

/** Places an object at its original logical coordinates. */
declare function position_origin(object: RuntimeObject): void;

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
 * asteroid.angle = random(0, 360);
 */
declare function random(min: number, max: number): number;

/**
 * Casts an invisible ray from an object using collision.with as group filter.
 */
declare function ray(
    source: RuntimeObject,
    angle: number,
    distance: number
): RayResult;

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
): void;

/**
 * Draws text on screen using logical screen coordinates.
 *
 * Coordinates are expressed in FLX logical resolution.
 * The engine applies the configured screen scale internally.
 * This is a screen-space helper; use shape.type = "text" for world text.
 *
 * @example
 * draw_text(10, 10, "SCORE: " + global["score"]);
 *
 * @example
 * draw_text(10, 25, "LIVES: " + global["lives"], 8);
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

/**
 * Draws one screen-space pixel using logical screen coordinates.
 */
declare function draw_pixel(
    x: number,
    y: number,
    color?: string
): void;

/**
 * Draws a screen-space line using logical screen coordinates.
 */
declare function draw_line(
    x: number,
    y: number,
    x1: number,
    y1: number,
    color?: string
): void;

/**
 * Draws a screen-space rectangle outline using logical screen coordinates.
 */
declare function draw_rectangle(
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
 * Called only when this object has collision.active = true
 * and the other object's group is listed in collision.with.
 */
declare function collision(
    object: RuntimeObject,
    other: RuntimeObject
): void;

/**
 * Optional draw phase function.
 */
declare function draw(object: RuntimeObject): void;

/**
 * Called once before an object is removed.
 * Executed after kill() and before cleanup.
 */
declare function dead(object: RuntimeObject): void;
