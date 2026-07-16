/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/* 
    The player moves using the up and down keys. 
*/

function action(player) {
    if (Input.player(1).up()) {
        move_y(player, UP);
    }

    if (Input.player(1).down()) {
        move_y(player, DOWN);
    }
}