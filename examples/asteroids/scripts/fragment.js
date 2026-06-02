/// <reference path="../../../../tools/scripts/flx.d.ts" />

const TIME_TO_DIE = 1.5;
 
function born(fragment) {
    fragment.local["timer"] = 0;
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

    fragment.local["timer"] += delta();

    if (fragment.local["timer"] < TIME_TO_DIE) {
        return;
    }

    if (probability(5)) {
        kill(fragment);
    }
}