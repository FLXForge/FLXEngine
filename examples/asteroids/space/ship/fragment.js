/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Ship fragments are explosion debris.
    Each fragment chooses a movement mode, shrinks over time and then disappears.
*/

const TIME_TO_DIE = 1.5;

function born(fragment) {
    play_timer(fragment, "life", TIME_TO_DIE);

    fragment.local["mode"] = random(0, 3);

    fragment.angle = random(0, 360);
    fragment.speed = random(10, 200);
    fragment.rotationSpeed = random(120, 360);
}

function motion(fragment) {
    if (fragment.local["mode"] < 1) {
        advance(fragment);
    } else if (fragment.local["mode"] < 2) {
        rotate(fragment, RIGHT);
    } else {
        advance(fragment);
        rotate(fragment, RIGHT);
    }

    const factor = timer_left(fragment, "life") / TIME_TO_DIE;

    fragment.width = 1 * factor;
    fragment.height = 10 * factor;

    if (!timer_active(fragment, "life")) {
        kill(fragment);
    }
}