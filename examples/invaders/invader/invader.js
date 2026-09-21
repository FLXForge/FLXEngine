/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Invader enemy.
    Moves with the shared fleet direction and dies when hit by a laser.
*/

function born(invader) {
    write_global("invaders", (read_global("invaders") || 0) + 1);
}

function motion(invader){
    if (read_global("game_over") == 1 || read_global("victory") == 1) {
        return;
    }

    if (probability(5, 10000)){
        spawn(invader, "laser");
    }
}

function collision(invader, other){
    if (other.group == "laser") {
        kill(other);
        kill(invader);

        write_global("score", read_global("score") + 100);
        write_global("invaders", read_global("invaders") - 1);
    }
}
