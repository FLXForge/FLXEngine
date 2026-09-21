/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

const MOVE = direction(0);
const FIRE = button(0);

function action(ship)
{
    const thrust = input_direction(ship, MOVE, VERTICAL);
    const turn = input_direction(ship, MOVE, HORIZONTAL);

    if (thrust > 0) {
        accelerate(ship, thrust);
    }

    if (turn != 0) {
        rotate(ship, turn);
    }

    if (input_pressed(ship, FIRE)) {
        spawn(ship, "probe");
    }

    if (ship.x < 0) position(ship, 320, ship.y);
    if (ship.x > 320) position(ship, 0, ship.y);
    if (ship.y < 0) position(ship, ship.x, 240);
    if (ship.y > 240) position(ship, ship.x, 0);
}
