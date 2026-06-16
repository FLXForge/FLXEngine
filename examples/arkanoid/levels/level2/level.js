/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

let introTime = 0;
let started = 0;

function born(level){
    fade_set(1);
    fade_off();
    introTime = 0;
    started = 0;
}

function action(level){

    if (started == 0){
        introTime += delta();

        if (introTime >= 2){
            started = 1;

            global["show_hud"] = 1;
            
            spawn(level, "background");
            spawn(level, "board");
            spawn(level, "paddle");
            spawn(level, "bricks");
        }
    }
}

function draw(level){

    if (started == 0){
        let value = Math.floor(180 + Math.sin(introTime * 6) * 60);
        let hex = value.toString(16).padStart(2, "0");

        draw_text(
            270,
            140,
            "LEVEL 2",
            28,
            "#" + hex + hex + "ff"
        );
    }
}