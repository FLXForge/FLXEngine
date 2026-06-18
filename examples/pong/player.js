/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/* 
    The player moves using the up and down keys. 
*/

function action(player) {
    if (Key.down(KEY_UP)) {
        move_y(player, UP);
    }

    if (Key.down(KEY_DOWN)) {
        move_y(player, DOWN);
    }
}