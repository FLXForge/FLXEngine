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
 * Runtime representation of an object created by FLX.
 */
interface RuntimeObject {
    /** Object name, usually defined in JSON. */
    name: string;

    /** Behavior identifier, usually defined in JSON. */
    behavior: string;

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

    /** Current movement speed. */
    speed: number;

    /** Initial movement speed. */
    originSpeed: number;

    /** Current movement angle, in degrees. */
    angle: number;

    /** Initial X position. */
    originX: number;

    /** Initial Y position. */
    originY: number;
}

/**
 * Keyboard input helper.
 */
declare const Key: {
    /** Returns true while the up key is pressed. */
    up(): boolean;

    /** Returns true while the down key is pressed. */
    down(): boolean;
};

/**
 * Moves an object on the X axis.
 *
 * @example
 * move_x(self, RIGHT);
 * move_x(self, LEFT * inertia);
 */
declare function move_x(self: RuntimeObject, direction: number): void;

/**
 * Moves an object on the Y axis.
 *
 * @example
 * move_y(self, UP);
 * move_y(self, DOWN * inertia);
 */
declare function move_y(self: RuntimeObject, direction: number): void;

/**
 * Moves an object using its angle and speed.
 *
 * @example
 * function motion(self) {
 *     advance(self);
 * }
 */
declare function advance(self: RuntimeObject): void;

/**
 * Makes an object follow another object on the Y axis.
 *
 * @example
 * follow_y(self, "ball");
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
 * @example
 * accelerate(self, 5);
 */
declare function accelerate(self: RuntimeObject, amount: number): void;

/**
 * Sends the object back to its origin and restores its initial speed.
 */
declare function to_origin(self: RuntimeObject): void;

/**
 * Optional lifecycle function.
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
