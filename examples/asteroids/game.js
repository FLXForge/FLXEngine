/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Defines the game.
*/

let stars = [];

function born(game) {
    write_global("score", 0);
    write_global("asteroids", 0);
    write_global("shipDead", 0);
    write_global("score", 0);
    write_global("inGame", 1);
    write_global("asteroids", 0);

    for(let i = 0; i < 100; i++){
        stars.push({
            x: random(0, 639),
            y: random(0, 319)
        });
    }
}

function motion(ui){
    if (read_global("shipDead") == 1) {
        spawn(ui, "ship");
        write_global("shipDead", 0);
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
