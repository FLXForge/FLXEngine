/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function born(game){
    write_global("score", 0);
    write_global("game_over", 0);
}

function draw(game){
    draw_text(70, 458, "SCORE: " + read_global("score"), 16, "white");

    if (read_global("game_over") == 1) {
        draw_text(320, 252, "GAME OVER", 24, "red");
    }
}