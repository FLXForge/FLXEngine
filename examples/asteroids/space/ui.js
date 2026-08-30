/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Defines the initial game state.
*/

// CONSTANTS
const INIT_LIVES = 3;
const FIRE_BUTTON = button(0);

function action(ui) {
    if (read_global("inGame") == 0) {
        if (input_pressed(player(1), FIRE_BUTTON)) {
            pause_music();
            write_global("score", 0);
            write_global("lives", INIT_LIVES);
            write_global("inGame", 1);
            write_global("asteroids", 0);

            spawn(ui, "ship");   //no usa origin, le he tenido que añadir el offset
        }
    }
}

function motion(ui){
    if (read_global("shipDead") == 1) {
        spawn(ui, "ship");   //no usa origin, le he tenido que añadir el offset
        write_global("shipDead", 0);
    }
}

function draw(ui){
    if (read_global("inGame") == 0) {
        if (read_global("lives") == 0){
            draw_text(
                250,
                100,
                "GAME OVER",
                20
            );

            draw_text(
                250,
                140,
                "SCORE: " + read_global("score"),
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
                "SCORE: " + read_global("score")
            );
        draw_text(
                10,
                20,
                "LIVES: " + read_global("lives")
            );
    }
}
