/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    The ship tail is a short-lived thrust trail.
    It moves forward, shrinks quickly and then disappears.
*/

const LIVE_TIME = 0.5;

function born(tail) {
    timer(tail, "life", LIVE_TIME);

    tail.speed = 10;
}

function motion(tail) {

    advance(tail);

    const factor = timer_left(tail, "life") / LIVE_TIME;

    tail.width = 6 * factor;
    tail.height = 8 * factor;

    if (!timer_active(tail, "life")) {
        kill(tail);
    }
}