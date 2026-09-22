/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    DeepSpace periodically creates asteroids.
*/

const SPAWN_INTERVAL = 1.5;
const ASTEROID_LIMIT = 10;

function born(space) {
    play_timer(space, "spawn", SPAWN_INTERVAL);
}

function motion(space) {
    if (timer_active(space, "spawn")) {
        return;
    }

    if (read_global("asteroids") >= ASTEROID_LIMIT) {
        play_timer(space, "spawn", SPAWN_INTERVAL);
        return;
    }

    if (probability(5)) {
        spawn(space, "Asteroid_c");
    } else if (probability(25)) {
        spawn(space, "Asteroid_b");
    } else {
        spawn(space, "Asteroid_a");
    }

    play_timer(space, "spawn", SPAWN_INTERVAL);
}
