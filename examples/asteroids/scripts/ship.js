/// <reference path="../../../../tools/scripts/flx.d.ts" />

/* 
    The player moves the ship using:
        -up key to accelerate with acceleration, inertia and max speed defined in motion
        -left and right keys to rotate using the rotation speed defined in motion
*/

function action(self) {

    if (Key.up()) {
        accelerate(self);
    }

    if (Key.left()) {
        rotate(self, LEFT);
    }

    if (Key.right()) {
        rotate(self, RIGHT);
    }
}

function motion(self) {
    advance(self);
}