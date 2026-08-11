/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Shared Arkanoid level controller.
    It shows the level intro, then creates the board, paddle and bricks.
*/

const INTRO_TIME = 2;

function born(level){
    fade_set(1);
    fade_off();

    timer(level, "intro", INTRO_TIME);
}

function action(level){
    if (state_active(level, "intro")) {
        if (!timer_active(level, "intro")) {
            global["show_hud"] = 1;

            spawn(level, "background");
            spawn(level, "board");
            spawn(level, "paddle");
            spawn(level, "bricks");

            state_to(level, "playing");
        }
    }
}

function draw(level){
    if (!state_active(level, "intro")) {
        return;
    }

    let t = INTRO_TIME - timer_left(level, "intro");

    let value =
        Math.floor(
            180 + Math.sin(t * 6) * 60
        );

    let hex =
        value
            .toString(16)
            .padStart(2, "0");

    draw_text(
        270,
        140,
        "LEVEL " + global["level"],
        28,
        "#" + hex + hex + "ff"
    );
}
