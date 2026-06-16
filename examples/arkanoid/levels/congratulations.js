/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

let screenTime = 0;
let end = 0;

function born(congratulations){
    screenTime = 0;
    end = 0;
}

function action(congratulations){
    if (end == 0){
        screenTime += delta();

        if (screenTime >= 6){
            end = 1;
            global["congratulations"] = 2;
        }
    }
}

function draw(congratulations){
    if (end == 0) {
        let value = Math.floor(180 + Math.sin(screenTime * 6) * 60);
        let hex = value.toString(16).padStart(2, "0");

        draw_text(
            170,
            140,
            "CONGRATULATIONS",
            28,
            "#" + "0000" + hex 
        );

        draw_text(
            260,
            200,
            "-- SCORE: " + global["score"] + " --",
            12
        );
    }
}