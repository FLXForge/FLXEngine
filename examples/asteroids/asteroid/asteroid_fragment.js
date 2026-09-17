/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Asteroid fragments fly away, rotate randomly and shrink until they disappear.
*/

const LIVE_TIME = 1.2;

function born(fragment) {
    play_timer(fragment, "life", LIVE_TIME);

    apply_angle(fragment, random(0, 360));
    apply_speed(fragment, random(20, 120));
    apply_rotation_speed(fragment, random(120, 360));
}

function motion(fragment) {
    advance(fragment);

    if (probability(50)) {
        rotate(fragment, RIGHT);
    }

    const factor = timer_left(fragment, "life") / LIVE_TIME;

    resize(fragment, 1 * factor, 10 * factor);

    if (!timer_active(fragment, "life")) {
        kill(fragment);
    }
}
