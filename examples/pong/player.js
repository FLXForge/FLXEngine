/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/* 
    The player moves using the up and down keys. 
*/

function action(player) {
    const MOVE = direction(0);

    if (input_down(player, MOVE, NEGATIVE)) {
        move_y(player, UP);
    }

    if (input_down(player, MOVE, POSITIVE)) {
        move_y(player, DOWN);
    }
}
