/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/* 
    The player moves the ship using:
        -up key to accelerate with acceleration, inertia and speed limit defined in mechanics
        -left and right keys to rotate using the rotation speed defined in mechanics
        -space key shoot (laser spawn)
*/

const FIRE_BUTTON = button(0);
const MOVE = direction(0);

function action(ship) {

    const thrust = input_direction(ship, MOVE, VERTICAL);
    const rotation = input_direction(ship, MOVE, HORIZONTAL);

    if (thrust > 0) {
        accelerate(ship, thrust);

        if (probability(20)) {
            play_sound(ship, "motor");
            spawn(ship, "tail");
        }
    }

    if (rotation != 0) {
        rotate(ship, rotation);
    }

    if (input_pressed(ship, FIRE_BUTTON)) {
        play_sound(ship, "laser");
        spawn(ship, "laser");
    }
}

function dead(ship) {
    play_sound(ship, "dead");
    if (read_global("lives") >= 2) {
        write_global("shipDead", 1);
        write_global("lives", read_global("lives") - 1);
    } else {
        write_global("inGame", 0);
        write_global("lives", 0);
        pause_music();
    }
    spawn(ship, "fragment");
    spawn(ship, "fragment");
    spawn(ship, "fragment");
}
