/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function draw(ui){

    if (ui.local["time"] == null) {
        ui.local["time"] = 0;
    }

    ui.local["time"] += delta();

    let intensity =
        Math.floor(
            64 +
            Math.sin(ui.local["time"] * 2) * 64
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