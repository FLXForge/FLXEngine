/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    The laser moves in its current direction and fades out before disappearing.
*/

const MAX_LIFE_TIME = 4;

function born(laser) {
    play_timer(laser, "life", MAX_LIFE_TIME);
}

function motion(laser) {

    advance(laser);

    const factor = timer_left(laser, "life") / MAX_LIFE_TIME;

    laser.width = 1 * factor;
    laser.height = 10 * factor;

    if (!timer_active(laser, "life")) {
        kill(laser);
    }
}