/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Defines the initial game state.
*/

// CONSTANTS
const INIT_LIVES = 3;

let stars = [];

function born(game) {
    global["score"] = 0;
    global["lives"] = INIT_LIVES;
    global["asteroids"] = 0;
    global["inGame"] = 0;
    global["shipDead"] = 0;

    for(let i = 0; i < 100; i++){

        stars.push({
            x: random(0, 639),
            y: random(0, 319)
        });
    }
}

function draw(space){

    for(let star of stars){

        draw_pixel(
            star.x,
            star.y,
            "white"
        );
    }
}