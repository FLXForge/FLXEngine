/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Title screen UI.
    Draws the blinking start message over the animated title scene.
*/

function born(ui){
    ui.local["time"] = 0;
}

function draw(ui){
    ui.local["time"] += delta();

    let intensity =
        Math.floor(
            64 + Math.sin(ui.local["time"] * 2) * 64
        );

    let hex =
        intensity
            .toString(16)
            .padStart(2, "0");

    draw_text(
        200,
        200,
        "-- Press SPACE to Start --",
        18,
        "#" + hex + hex + "ff"
    );
}