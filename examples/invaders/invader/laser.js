/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Player laser.
    Travels upward and disappears outside the screen.
*/

function motion(laser){
    advance(laser);

    if (laser.y > 450) {
        kill(laser);
    }
}
