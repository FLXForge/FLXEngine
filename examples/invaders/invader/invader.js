/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Invader enemy.
    Fires lasers and dies when hit by the player.
*/

function motion(invader){
    if (read_global("game_over") == 1) {
        return;
    }

    if (probability(5, 10000)){
        spawn(invader, "laser");
    }
}
