/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Invaders game controller.
    Keeps global score and detects victory or game over.
*/

function born(game){
    global["score"] = 0;
    global["invaders"] = 0;
    global["game_over"] = 0;
    global["victory"] = 0;

    global["fleet_direction"] = RIGHT;
    global["fleet_drop"] = 0;
}

function action(game){
    if (global["game_over"] == 1 || global["victory"] == 1) {
        return;
    }

    if (global["invaders"] <= 0) {
        global["victory"] = 1;
    }
}

function draw(game){
    draw_text(10, 450, "SCORE: " + global["score"], 16, "white");

    if (global["game_over"] == 1) {
        draw_text(250, 240, "GAME OVER", 24, "red");
    }

    if (global["victory"] == 1) {
        draw_text(250, 240, "VICTORY", 24, "green");
    }
}