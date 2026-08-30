/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Invader enemy.
    Moves with the shared fleet direction and dies when hit by a laser.
*/

let speed = 10;

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

    apply_speed(invader, speed);
    move_horizontal(invader, read_global("fleet_direction"));

    if (invader.x < 20) {
        write_global("fleet_direction", RIGHT);
        write_global("fleet_drop", read_global("invaders"));
    }

    if (invader.x > 600) {
        write_global("fleet_direction", LEFT);
        write_global("fleet_drop", read_global("invaders"));
    }

    if (read_global("fleet_drop") >= 1) {
        position_y(invader, invader.y + 8);
        write_global("fleet_drop", read_global("fleet_drop") - 1);
    }

    if (invader.y > 450) {
        write_global("game_over", 1);
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

function dead(invader){
    speed += 0.6;
}
