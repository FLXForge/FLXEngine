/// <reference path="../../../../tools/scripts/flx.d.ts" />

/*
    DeepSpace periodically spawns asteroids.
    Each spawner keeps its own timer and local asteroid limit.
*/

const SPAWN_INTERVAL = 1.5;

const ASTEROID_LIMIT = 10;

function born(space) {
    space.local["timer"] = 0;
}

function motion(space) {
    if (global["inGame"] == 0) {
        return;
    }

    space.local["timer"] += delta();

    if (space.local["timer"] < SPAWN_INTERVAL) {
        return;
    }

    space.local["timer"] = 0;

    if (global["asteroids"] >= ASTEROID_LIMIT) {
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
    space.local["timer"] = 0;
}