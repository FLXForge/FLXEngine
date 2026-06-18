/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    DeepSpace periodically creates asteroids while the game is active.
    The timer is reset while the game is stopped so spawning starts cleanly.
*/

const SPAWN_INTERVAL = 1.5;
const ASTEROID_LIMIT = 10;

function born(space) {
    timer(space, "spawn", SPAWN_INTERVAL);
}

function motion(space) {
    if (global["inGame"] == 0) {
        timer(space, "spawn", SPAWN_INTERVAL);
        return;
    }

    if (timer_active(space, "spawn")) {
        return;
    }

    if (global["asteroids"] >= ASTEROID_LIMIT) {
        timer(space, "spawn", SPAWN_INTERVAL);
        return;
    }

    if (probability(5)) {
        spawn(space, "Asteroid_c");
    } else if (probability(25)) {
        spawn(space, "Asteroid_b");
    } else {
        spawn(space, "Asteroid_a");
    }

    global["asteroids"] += 2;

    timer(space, "spawn", SPAWN_INTERVAL);
}