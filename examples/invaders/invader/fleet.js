/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

let direction = RIGHT;

function motion(fleet) {
    if (read_global("game_over") == 1) {
        return;
    }

    move_horizontal(fleet, direction);

    if (fleet.x < 70) {
        direction = RIGHT;
        position_y(fleet, fleet.y + 8);
    }

    if (fleet.x > 190) {
        direction = LEFT;
        position_y(fleet, fleet.y + 8);
    }

    if (fleet.y > 450) {
        write_global("game_over", 1);
    }
}