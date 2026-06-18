// FLX Engine JavaScript API
// Version: 0.0.1
//
// Provides IntelliSense/documentation support for FLX scripts.
//
// Usage:
// /// <reference path="../flx.d.ts" />

/** Semantic direction constant. */
declare const UP: number;

/** Semantic direction constant. */
declare const DOWN: number;

/** Semantic direction constant. */
declare const LEFT: number;

/** Semantic direction constant. */
declare const RIGHT: number;

/** Neutral movement constant. */
declare const STOP: number;

/** Keyboard key constant. */
declare const KEY_UP: number;

/** Keyboard key constant. */
declare const KEY_DOWN: number;

/** Keyboard key constant. */
declare const KEY_LEFT: number;

/** Keyboard key constant. */
declare const KEY_RIGHT: number;

/** Keyboard key constant. */
declare const KEY_SPACE: number;

/**
 * Motion configuration exposed from JSON.
 */
interface MotionConfig {
    /** Initial movement speed. */
    speed: number;

    /** Initial movement angle in degrees. */
    angle: number;

    /** Rotation speed in degrees per second. */
    rotationSpeed: number;

    /** Acceleration applied when accelerate(object) is called. */
    acceleration: number;

    /** Velocity multiplier applied by advance(object). */
    inertia: number;

    /** Maximum vector speed. A value of 0 means no limit. */
    maxSpeed: number;
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

    /** Draw layer. Lower values are drawn first. */
    layer: number;

    /** True while the object is attached to its original parent. */
    attached: boolean;

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

    /** Initial X position. */
    originX: number;

    /** Initial Y position. */
    originY: number;

    /** Motion configuration defined in JSON. */
    motion: MotionConfig;
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

/**
 * Keyboard input helper.
 */
declare const Key: {
    /**
     * Returns true while the given key is pressed.
     *
     * @example
     * Key.down(KEY_UP)
     */
    down(key: number): boolean;

    /**
     * Returns true only on the frame the given key is pressed.
     *
     * @example
     * Key.pressed(KEY_SPACE)
     */
    pressed(key: number): boolean;
};

/**
 * Moves an object on the X axis.
 */
declare function move_x(object: RuntimeObject, direction: number): void;

/**
 * Moves an object on the Y axis.
 */
declare function move_y(object: RuntimeObject, direction: number): void;

/**
 * Moves an object.
 *
 * If motion.acceleration is defined, advance uses velocityX and velocityY.
 * Otherwise, it uses classic speed + angle movement.
 */
declare function advance(object: RuntimeObject): void;

/**
 * Rotates an object using motion.rotationSpeed.
 *
 * @example
 * rotate(object, LEFT);
 * rotate(object, RIGHT);
 */
declare function rotate(object: RuntimeObject, direction: number): void;

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
 * Applies a horizontal bounce by modifying the object's angle.
 */
declare function bounce_x(object: RuntimeObject): void;

/**
 * Applies a vertical bounce by modifying the object's angle.
 */
declare function bounce_y(object: RuntimeObject): void;

/**
 * Increases the object's speed by the given amount.
 *
 * Classic mode.
 */
declare function accelerate(object: RuntimeObject, amount: number): void;

/**
 * Accelerates the object using motion.acceleration, motion.maxSpeed
 * and the current angle.
 *
 * Motion-based mode.
 */
declare function accelerate(object: RuntimeObject): void;

/**
 * Sends the object back to its origin and restores its initial speed.
 */
declare function to_origin(object: RuntimeObject): void;

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
 * Changes the current state of an object using its JSON states declaration.
 */
declare function state(object: RuntimeObject, stateName: string): void;

/**
 * Returns the current state name, or an empty string if the object has none.
 */
declare function state_current(object: RuntimeObject): string;

/**
 * Returns true when the object is currently in the given state.
 */
declare function state_active(
    object: RuntimeObject,
    stateName: string
): boolean;

/**
 * Returns true only during the frame in which the object entered its state.
 */
declare function state_entered(object: RuntimeObject): boolean;

/**
 * Returns seconds elapsed since the object entered its current state.
 */
declare function state_time(object: RuntimeObject): number;

/**
 * Creates or restarts a named timer owned by the object.
 *
 * Finished timers stay stored with 0 seconds left until they are restarted,
 * cleared with timer_clear(), or their owner is destroyed.
 */
declare function timer(
    object: RuntimeObject,
    timerName: string,
    duration: number
): void;

/**
 * Returns true while the named object timer exists and has time left.
 */
declare function timer_active(
    object: RuntimeObject,
    timerName: string
): boolean;

/**
 * Returns remaining seconds for the named object timer, or 0 if it does not
 * exist or has already finished.
 */
declare function timer_left(
    object: RuntimeObject,
    timerName: string
): number;

/**
 * Removes a named object timer immediately.
 */
declare function timer_clear(
    object: RuntimeObject,
    timerName: string
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
