/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function motion(missile) {
    advance(missile);
}

function collision(missile, other) {

    if (other.group == "player") {
        kill(other);
    }

    kill(missile);
}