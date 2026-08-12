/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Player cannon.
    Moves horizontally and fires one laser with a short cooldown.
*/

const SHOT_COOLDOWN = 0.35;

function action(player){
    if (global["game_over"] == 1 || global["victory"] == 1) {
        return;
    }

    if (Key.down(KEY_LEFT)) {
        move_x(player, LEFT);
    }

    if (Key.down(KEY_RIGHT)) {
        move_x(player, RIGHT);
    }

    if (player.x < 20) {
        player.x = 20;
    }

    if (player.x > 596) {
        player.x = 596;
    }

    if (Key.pressed(KEY_SPACE) && !timer_active(player, "shot")) {
        spawn(player, "laser");
        play_timer(player, "shot", SHOT_COOLDOWN);
    }
}

function collision(player, other){

}