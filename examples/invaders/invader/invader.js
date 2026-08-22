/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Invader enemy.
    Moves with the shared fleet direction and dies when hit by a laser.
*/

let speed = 10;

function born(invader) {
    global["invaders"]++;
}

function motion(invader){
    if (global["game_over"] == 1 || global["victory"] == 1) {
        return;
    }

    if (probability(5, 10000)){
        spawn(invader, "laser");
    }

    apply_speed(invader, speed);
    move_horizontal(invader, global["fleet_direction"]);

    if (invader.x < 20) {
        global["fleet_direction"] = RIGHT;
        global["fleet_drop"] = global["invaders"];
    }

    if (invader.x > 600) {
        global["fleet_direction"] = LEFT;
        global["fleet_drop"] = global["invaders"];
    }

    if (global["fleet_drop"] >= 1) {
        invader.y += 8;
        global["fleet_drop"] --;
    }

    if (invader.y > 450) {
        global["game_over"] = 1;
    }
}

function collision(invader, other){
    if (other.group == "laser") {
        kill(other);
        kill(invader);

        global["score"] += 100;
        global["invaders"] -= 1;
    }
}

function dead(invader){
    speed += 0.6;
}
