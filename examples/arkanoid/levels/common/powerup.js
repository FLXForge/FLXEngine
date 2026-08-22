/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Arkanoid power-up.
    It falls down and disappears when it leaves the screen.
*/

function motion(powerup){
    move_vertical(powerup, 1);

    if (powerup.y > 320) {
        kill(powerup);
    }
}
