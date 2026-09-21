/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Player laser.
    Travels upward and disappears outside the screen.
*/

function motion(laser){
    advance(laser);

    if (laser.y > 490) {
        kill(laser);
    }
}

function collision(laser, invader){
    kill(laser);
    kill(invader);

    write_global("score", read_global("score") + 100);
}