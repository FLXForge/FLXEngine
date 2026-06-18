/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

let screenTime = 0;
let end = 0;

function born(gameover){
    screenTime = 0;
    end = 0;
}

function action(gameover){
    if (end == 0){
        screenTime += delta();

        if (screenTime >= 2){
            end = 1;
            global["game_over"] = 3;
        }
    }
}

function draw(gameover){
    if (end == 0) {
        let value = Math.floor(180 + Math.sin(screenTime * 6) * 60);
        let hex = value.toString(16).padStart(2, "0");

        draw_text(
            240,
            140,
            "GAME OVER",
            28,
            "#" + hex + "0000"
        );

        draw_text(
            260,
            200,
            "-- SCORE: " + global["score"] + " --",
            12
        );
    }
}