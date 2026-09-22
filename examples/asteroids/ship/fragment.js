/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Ship fragments are explosion debris.
    Each fragment chooses a movement mode, shrinks over time and then disappears.
*/

const TIME_TO_DIE = 1.5;

function born(fragment) {
    play_timer(fragment, "life", TIME_TO_DIE);

    write_local(fragment, "spin", random(0, 100) < 50 ? LEFT : RIGHT);
    apply_rotation_speed(fragment, random(120, 360));
}

function motion(fragment) {
    rotate(fragment, read_local(fragment, "spin"));

    const factor = timer_left(fragment, "life") / TIME_TO_DIE;

    resize(fragment, 1 * factor, 10 * factor);

    if (!timer_active(fragment, "life")) {
        kill(fragment);
    }
}
