/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

const LIVE_TIME = 0.5;

function born(tail) {
    tail.local["timer"] = 0;
    tail.speed = 10;
}

function motion(tail) {
    tail.local["timer"] += delta();

    advance(tail);

    //scale {
    const factor = 1 - tail.local["timer"] / LIVE_TIME;

    tail.width = 6 * factor;
    tail.height = 8 * factor;
    // }

    if (tail.local["timer"] >= LIVE_TIME) {
        kill(tail);
    }
}