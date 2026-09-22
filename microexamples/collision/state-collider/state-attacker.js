/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function action(attacker) {
    if (state_current(attacker) === "idle" && attacker.x >= 115) {
        state_to(attacker, "attack");
    }
}

function motion(attacker) {
    advance(attacker);
}

function collision(attacker, other, contacts) {
    if (other.group !== "enemy") {
        return;
    }

    for (const contact of contacts) {
        if (contact.collider === "sword") {
            kill(other);
            return;
        }
    }
}
