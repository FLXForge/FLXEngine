/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/* 
    The player moves the ship using:
        -up key to accelerate with acceleration, inertia and max speed defined in motion
        -left and right keys to rotate using the rotation speed defined in motion
        -space key shoot (laser spawn)
*/

const FIRE_BUTTON = button(0);
const MOVE = direction(0);

function action(ship) {

    if (input_down(ship, MOVE, UP)) {
        accelerate(ship);

        if (probability(20)) {
            play_sound(ship, "motor");
            spawn(ship, "tail");
        }
    }

    if (input_down(ship, MOVE, LEFT)) {
        rotate(ship, LEFT);
    }

    if (input_down(ship, MOVE, RIGHT)) {
        rotate(ship, RIGHT);
    }

    if (input_pressed(ship, FIRE_BUTTON)) {
        play_sound(ship, "laser");
        spawn(ship, "laser");
    }
}

function motion(ship) {
    advance(ship);
}

function dead(ship) {
    play_sound(ship, "dead");
    if (global["lives"] >= 2) {
        global["shipDead"] = 1;
        global["lives"]--;
    } else {
        global["inGame"] = 0;
        global["lives"] = 0;
        pause_music();
    }
    spawn(ship, "fragment");
    spawn(ship, "fragment");
    spawn(ship, "fragment");
}
