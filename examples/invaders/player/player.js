/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Player cannon.
    Moves horizontally and fires one laser with a short cooldown.
*/

const SHOT_COOLDOWN = 0.35;
const FIRE_BUTTON = button(0);
const MOVE = direction(0);

function action(player){
    if (read_global("game_over") == 1 || read_global("victory") == 1) {
        return;
    }

    move_horizontal(player, input_direction(player, MOVE, HORIZONTAL));

    if (player.x < 20) {
        position_x(player, 20);
    }

    if (player.x > 596) {
        position_x(player, 596);
    }

    if (input_pressed(player, FIRE_BUTTON) && !timer_active(player, "shot")) {
        spawn(player, "laser");
        play_timer(player, "shot", SHOT_COOLDOWN);
    }
}

function collision(player, other){

}
