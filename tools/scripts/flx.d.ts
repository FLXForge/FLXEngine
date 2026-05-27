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

/**
 * Motion configuration exposed from JSON.
 */
interface MotionConfig {
    /** Rotation speed in degrees per second. */
    rotationSpeed: number;

    /** Acceleration applied when accelerate(self) is called. */
    acceleration: number;

    /** Velocity multiplier applied by advance(self). */
    inertia: number;

    /** Maximum vector speed. A value of 0 means no limit. */
    maxSpeed: number;
}

/**
 * Runtime representation of an object created by FLX.
 */
interface RuntimeObject {
    /** Object name, usually defined in JSON. */
    name: string;

    /** Collision group identifier. */
    group: string;

    /** Current X position. */
    x: number;

    /** Current Y position. */
    y: number;

    /** Runtime object width. */
    width: number;

    /** Runtime object height. */
    height: number;

    /** Current movement speed. Used by classic speed + angle movement. */
    speed: number;

    /** Initial movement speed. */
    originSpeed: number;

    /** Current movement angle, in degrees. */
    angle: number;

    /** Current X velocity. Used by motion-based movement. */
    velocityX: number;

    /** Current Y velocity. Used by motion-based movement. */
    velocityY: number;

    /** Initial X position. */
    originX: number;

    /** Initial Y position. */
    originY: number;

    /** Motion configuration defined in JSON. */
    motion: MotionConfig;
}

/**
 * Keyboard input helper.
 */
declare const Key: {
    /** Returns true while the up key is pressed. */
    up(): boolean;

    /** Returns true while the down key is pressed. */
    down(): boolean;

    /** Returns true while the left key is pressed. */
    left(): boolean;

    /** Returns true while the right key is pressed. */
    right(): boolean;
};

/**
 * Moves an object on the X axis.
 */
declare function move_x(self: RuntimeObject, direction: number): void;

/**
 * Moves an object on the Y axis.
 */
declare function move_y(self: RuntimeObject, direction: number): void;

/**
 * Moves an object.
 *
 * If motion.acceleration is defined, advance uses velocityX and velocityY.
 * Otherwise, it uses classic speed + angle movement.
 */
declare function advance(self: RuntimeObject): void;

/**
 * Rotates an object using motion.rotationSpeed.
 *
 * @example
 * rotate(self, LEFT);
 * rotate(self, RIGHT);
 */
declare function rotate(self: RuntimeObject, direction: number): void;

/**
 * Makes an object follow another object on the Y axis.
 */
declare function follow_y(self: RuntimeObject, targetName: string): void;

/**
 * Applies a horizontal bounce by modifying the object's angle.
 */
declare function bounce_x(self: RuntimeObject): void;

/**
 * Applies a vertical bounce by modifying the object's angle.
 */
declare function bounce_y(self: RuntimeObject): void;

/**
 * Increases the object's speed by the given amount.
 *
 * Classic mode.
 */
declare function accelerate(self: RuntimeObject, amount: number): void;

/**
 * Accelerates the object using motion.acceleration, motion.maxSpeed
 * and the current angle.
 *
 * Motion-based mode.
 */
declare function accelerate(self: RuntimeObject): void;

/**
 * Sends the object back to its origin and restores its initial speed.
 */
declare function to_origin(self: RuntimeObject): void;

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
 * Optional game lifecycle function.
 */
declare function gameStart(): void;

/**
 * Optional object lifecycle function.
 */
declare function start(self: RuntimeObject): void;

/**
 * Optional action phase function.
 */
declare function action(self: RuntimeObject): void;

/**
 * Optional motion phase function.
 */
declare function motion(self: RuntimeObject): void;

/**
 * Optional collision phase function.
 */
declare function collision(self: RuntimeObject, other: RuntimeObject): void;

/**
 * Optional draw phase function.
 */
declare function draw(self: RuntimeObject): void;