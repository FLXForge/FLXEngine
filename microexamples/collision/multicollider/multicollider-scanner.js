/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function motion(scanner) {
    advance(scanner);
}

function collision(scanner, other, contacts) {
    if (other.group !== "enemy") {
        return;
    }

    for (const contact of contacts) {
        if (contact.collider === "probe") {
            // "probe" and "body" belong to the same RuntimeObject.
            // contacts[] tells us which collider actually participated.
            kill(other);
            kill(scanner);
            return;
        }
    }
}
