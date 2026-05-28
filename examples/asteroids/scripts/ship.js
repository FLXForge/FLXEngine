/// <reference path="../../../../tools/scripts/flx.d.ts" />

/* 
    The player moves the ship using:
        -up key to accelerate with acceleration, inertia and max speed defined in motion
        -left and right keys to rotate using the rotation speed defined in motion
        -space key shoot (laser spawn)
*/

function action(ship) {

    if (Key.down(KEY_UP)) {
        accelerate(ship);
    }

    if (Key.down(KEY_LEFT)) {
        rotate(ship, LEFT);
    }

    if (Key.down(KEY_RIGHT)) {
        rotate(ship, RIGHT);
    }

    if (Key.pressed(KEY_SPACE)) {
        spawn(ship, "laser");
    }
}

function motion(ship) {
    advance(ship);
}