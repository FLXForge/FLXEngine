/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

const MOVE = direction(0);
const FIRE = button(0);

function action(tank) {

    const forward =
        -input_direction(tank, MOVE, VERTICAL);

    const rotation =
        input_direction(tank, MOVE, HORIZONTAL);

    if (rotation != 0) {
        rotate(tank, rotation);
    }

    if (forward > 0) {
        advance(tank);
    }

    if (input_pressed(tank, FIRE)) {
        spawn(tank, "missile");
    }
}

function collision(tank, other, contacts) {
    for (const contact of contacts) {
        position(tank, contact);
    }
}
