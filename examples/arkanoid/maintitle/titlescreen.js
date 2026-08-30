/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Title screen UI.
    Draws the blinking start message over the animated title scene.
*/

function born(ui){
    write_local(ui, "time", 0);
}

function draw(ui){
    write_local(ui, "time", read_local(ui, "time") + delta());

    let intensity =
        Math.floor(
            64 + Math.sin(read_local(ui, "time") * 2) * 64
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
