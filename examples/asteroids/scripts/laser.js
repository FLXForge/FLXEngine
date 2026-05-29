/// <reference path="../../../../tools/scripts/flx.d.ts" />

/* 
    The laser moves in the direction of the father and dead in 4 seconds
*/

const MAX_TIME_LIFE = 4;

let laser_counter = 0;

function born(laser) {
    laser.local["life_time"] = 0;
    laser_counter++;
    laser.local["laser_number"] = laser_counter;
    //console.log("Laser " + laser.local["laser_number"] +" born");
}

function motion(laser) {
    advance(laser);

    laser.local["life_time"] += delta();

    if (laser.local["life_time"] >= MAX_TIME_LIFE) {
        kill(laser);
    }
}

function dead(laser) {
    //console.log("Laser " + laser.local["laser_number"] + " dead");
}