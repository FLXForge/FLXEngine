/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Invaders game controller.
    Keeps global score and detects victory or game over.
*/

function born(game){
    write_global("score", 0);
    write_global("invaders", 0);
    write_global("game_over", 0);
    write_global("victory", 0);

    write_global("fleet_direction", RIGHT);
    write_global("fleet_drop", 0);
}

function action(game){
    if (read_global("game_over") == 1 || read_global("victory") == 1) {
        return;
    }

    if (read_global("invaders") <= 0) {
        write_global("victory", 1);
    }
}

function draw(game){
    draw_text(10, 450, "SCORE: " + read_global("score"), 16, "white");

    if (read_global("game_over") == 1) {
        draw_text(250, 240, "GAME OVER", 24, "red");
    }

    if (read_global("victory") == 1) {
        draw_text(250, 240, "VICTORY", 24, "green");
    }
}
