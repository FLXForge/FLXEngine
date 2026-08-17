/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Player cannon.
    Moves horizontally and fires one laser with a short cooldown.
*/

const SHOT_COOLDOWN = 0.35;
const FIRE_BUTTON = button(0);
const MOVE = direction(0);

function action(player){
    if (global["game_over"] == 1 || global["victory"] == 1) {
        return;
    }

    if (input_down(player, MOVE, LEFT)) {
        move_x(player, LEFT);
    }

    if (input_down(player, MOVE, RIGHT)) {
        move_x(player, RIGHT);
    }

    if (player.x < 20) {
        player.x = 20;
    }

    if (player.x > 596) {
        player.x = 596;
    }

    if (input_pressed(player, FIRE_BUTTON) && !timer_active(player, "shot")) {
        spawn(player, "laser");
        play_timer(player, "shot", SHOT_COOLDOWN);
    }
}

function collision(player, other){

}
