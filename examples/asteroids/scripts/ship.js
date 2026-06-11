/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/* 
    The player moves the ship using:
        -up key to accelerate with acceleration, inertia and max speed defined in motion
        -left and right keys to rotate using the rotation speed defined in motion
        -space key shoot (laser spawn)
*/

function action(ship) {

    if (Key.down(KEY_UP)) {
        accelerate(ship);

        if (probability(20)) {
            spawn(ship, "tail");
        }
    }

    if (Key.down(KEY_LEFT)) {
        rotate(ship, LEFT);
    }

    if (Key.down(KEY_RIGHT)) {
        rotate(ship, RIGHT);
    }

    if (Key.pressed(KEY_SPACE)) {
        play_sound(ship, "laser");
        spawn(ship, "laser");
    }
}

function motion(ship) {
    advance(ship);
}

function dead(ship) {
    if (global["lives"] >= 2) {
        global["shipDead"] = 1;
        global["lives"]--;
    } else {
        global["inGame"] = 0;
        global["lives"] = 0;
    }
    spawn(ship, "fragment");
    spawn(ship, "fragment");
    spawn(ship, "fragment");
}
