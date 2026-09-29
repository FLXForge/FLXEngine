/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Defines the ui game.
*/

function draw(ui){
        draw_text(
                55,
                15,
                "SCORE: " + read_global("score"),
                10
            );
}
