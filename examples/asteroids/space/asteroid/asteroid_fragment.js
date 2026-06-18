/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Asteroid fragments fly away, rotate randomly and shrink until they disappear.
*/

const LIVE_TIME = 1.2;

function born(fragment) {
    timer(fragment, "life", LIVE_TIME);

    fragment.angle = random(0, 360);
    fragment.speed = random(20, 120);
    fragment.rotationSpeed = random(120, 360);
}

function motion(fragment) {
    advance(fragment);

    if (probability(50)) {
        rotate(fragment, RIGHT);
    }

    const factor = timer_left(fragment, "life") / LIVE_TIME;

    fragment.width = 1 * factor;
    fragment.height = 10 * factor;

    if (!timer_active(fragment, "life")) {
        kill(fragment);
    }
}