/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Arkanoid power-up.
    It falls down and disappears when it leaves the screen.
*/

function motion(powerup){
    move_y(powerup, DOWN);

    if (powerup.y > 320) {
        kill(powerup);
    }
}