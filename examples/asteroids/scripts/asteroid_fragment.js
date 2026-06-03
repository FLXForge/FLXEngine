/// <reference path="../../../../tools/scripts/flx.d.ts" />

const LIVE_TIME = 1.2;

function born(fragment) {
    fragment.local["timer"] = 0;

    fragment.angle = random(0, 360);
    fragment.speed = random(20, 120);
    fragment.rotationSpeed = random(120, 360);
}

function motion(fragment) {
    fragment.local["timer"] += delta();

    advance(fragment);

    if (probability(50)) {
        rotate(fragment, RIGHT);
    }

    const factor = 1 - fragment.local["timer"] / LIVE_TIME;

    fragment.width = 1 * factor;
    fragment.height = 10 * factor;

    if (fragment.local["timer"] >= LIVE_TIME) {
        kill(fragment);
    }
}