/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Enemy laser.
    Travels downward and disappears outside the playfield.
*/

function motion(laser){
    advance(laser);

    if (laser.y > 450) {
        kill(laser);
    }
}
