/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Move and reaction with other asteroid
*/

function motion(asteroid) {
    advance(asteroid);
    rotate(asteroid);
}

function collision(asteroid, other, contacts) {
    for (const contact of contacts) {
        position(asteroid, contact);

        if (Math.abs(contact.normalX) > Math.abs(contact.normalY)) {
            reflect_x(asteroid);
        } else {
            reflect_y(asteroid);
        }
    }
}