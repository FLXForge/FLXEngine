/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Defines the game.
*/

let stars = [];

function born(game) {
    write_global("score", 0);
    write_global("asteroids", 0);
    write_local(game, "ship", true);

    for(let i = 0; i < 100; i++){
        stars.push({
            x: random(0, 639),
            y: random(0, 319)
        });
    }
}

function motion(game){
    if (!read_local(game, "ship")) {
        spawn(game, "ship");
        write_local(game, "ship", true);
    }
}

function draw(game){
    for(let star of stars){
        draw_pixel(
            star.x,
            star.y,
            "white"
        );
    }
}
