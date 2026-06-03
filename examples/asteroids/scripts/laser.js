/// <reference path="../../../../tools/scripts/flx.d.ts" />

/*
    The laser moves in its current direction and dies after 4 seconds.
*/

const MAX_TIME_LIFE = 4;

let laser_counter = 0;

function born(laser) {
    laser.local["life_time"] = 0;
    laser_counter++;
    laser.local["laser_number"] = laser_counter;
}

function motion(laser) {
    advance(laser);

    laser.local["life_time"] += delta();

    const factor = 1 - laser.local["life_time"] / (MAX_TIME_LIFE * 2);

    laser.width = 1 * factor;
    laser.height = 10 * factor;

    if (laser.local["life_time"] >= MAX_TIME_LIFE) {
        kill(laser);
    }
}

function dead(laser) {

}