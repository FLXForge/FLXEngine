/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Defines the initial game state.
*/

// CONSTANTS
const INIT_LIVES = 3;

function action(ui) {
    if (global["inGame"] == 0) {
        if (Key.down(KEY_SPACE)) {
            global["score"] = 0;
            global["lives"] = INIT_LIVES;
            global["inGame"] = 1;
            global["asteroids"] = 0;

            spawn(ui, "ship");   //no usa origin, le he tenido que añadir el offset
        }
    }
}

function motion(ui){
    if (global["shipDead"] == 1) {
        spawn(ui, "ship");   //no usa origin, le he tenido que añadir el offset
        global["shipDead"] = 0;
    }
}

function draw(ui){
    if (global["inGame"] == 0) {
        if (global["lives"] == 0){
            draw_text(
                250,
                100,
                "GAME OVER",
                20
            );

            draw_text(
                250,
                140,
                "SCORE: " + global["score"],
                15
            );
        }
	    draw_text(
            270,
            180,
            "-- PRESS SPACE TO START --"
        );
    } else {
        draw_text(
                10,
                10,
                "SCORE: " + global["score"]
            );
        draw_text(
                10,
                20,
                "LIVES: " + global["lives"]
            );
    }
}