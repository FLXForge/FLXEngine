/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    The ship tail is a short-lived thrust trail.
    It moves forward, shrinks quickly and then disappears.
*/

const LIVE_TIME = 0.5;

function born(tail) {
    play_timer(tail, "life", LIVE_TIME);

    apply_speed(tail, 10);
}

function motion(tail) {

    advance(tail);

    const factor = timer_left(tail, "life") / LIVE_TIME;

    resize(tail, 6 * factor, 8 * factor);

    if (!timer_active(tail, "life")) {
        kill(tail);
    }
}
