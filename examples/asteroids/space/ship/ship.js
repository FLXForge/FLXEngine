/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/* 
    The player moves the ship using:
        -up key to accelerate with acceleration, inertia and max speed defined in motion
        -left and right keys to rotate using the rotation speed defined in motion
        -space key shoot (laser spawn)
*/

const FIRE_BUTTON = 0;

function action(ship) {

    if (Input.player(1).up()) {
        accelerate(ship);

        if (probability(20)) {
            play_sound(ship, "motor");
            spawn(ship, "tail");
        }
    }

    if (Input.player(1).left()) {
        rotate(ship, LEFT);
    }

    if (Input.player(1).right()) {
        rotate(ship, RIGHT);
    }

    if (Input.player(1).pressed(FIRE_BUTTON)) {
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
