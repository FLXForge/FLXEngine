/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    DeepSpace periodically creates asteroids while the game is active.
    The timer is reset while the game is stopped so spawning starts cleanly.
*/

const SPAWN_INTERVAL = 1.5;
const ASTEROID_LIMIT = 10;

function born(space) {
    play_timer(space, "spawn", SPAWN_INTERVAL);
}

function motion(space) {
    if (read_global("inGame") == 0) {
        stop_timer(space, "spawn");
        play_timer(space, "spawn", SPAWN_INTERVAL);
        return;
    }

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

    write_global("asteroids", read_global("asteroids") + 2);

    play_timer(space, "spawn", SPAWN_INTERVAL);
}
