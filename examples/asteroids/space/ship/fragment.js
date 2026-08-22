/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Ship fragments are explosion debris.
    Each fragment chooses a movement mode, shrinks over time and then disappears.
*/

const TIME_TO_DIE = 1.5;

function born(fragment) {
    play_timer(fragment, "life", TIME_TO_DIE);

    fragment.local["spin"] = random(0, 100) < 50 ? LEFT : RIGHT;
    fragment.rotationSpeed = random(120, 360);
}

function motion(fragment) {
    rotate(fragment, fragment.local["spin"]);

    const factor = timer_left(fragment, "life") / TIME_TO_DIE;

    fragment.width = 1 * factor;
    fragment.height = 10 * factor;

    if (!timer_active(fragment, "life")) {
        kill(fragment);
    }
}
