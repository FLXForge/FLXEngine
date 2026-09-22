/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/* 
    The player moves using the up and down keys. 
*/

function action(player) {
    const MOVE = direction(0);
    move_vertical(player, -input_direction(player, MOVE, VERTICAL));
}
