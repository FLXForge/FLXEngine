/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function born(ball){
    ball.local["detachCooldown"] = 0;
}

function motion(ball){
    if (ball.local["detachCooldown"] > 0) {
        ball.local["detachCooldown"] -= delta();
    }

    if (ball.attached) {
        if (Key.pressed(KEY_SPACE)) {
            detach(ball);
            ball.angle = 135;
            ball.local["detachCooldown"] = 0.2;
        }
    } else {
	    advance(ball);

        if (ball.y > 320) {
            global["lives"] -= 1;
            global["ball_lost"] = 1;

            play_sound(ball, "lost");
            kill(ball);
        }
    }
}

function collision(ball, other) {
    if (!ball.attached) {
        if (other.group == "wall_top") {
            bounce_y(ball);
            play_sound(ball, "paddle");
        } else if (other.group == "wall_side") {
            bounce_x(ball);
            play_sound(ball, "paddle");
        } else if (other.group == "paddle") {
            if ( global["activeGun"] == 1 && ball.local["detachCooldown"] <= 0 ){
                attach(ball);
                return;
            }
             ball.y = other.y - ball.height - 1;

            let center = other.x + other.width / 2;
            let hit = ball.x - center;

            let factor = hit / (other.width / 2);

            ball.angle = 0 + factor * 60;

            play_sound(ball, "paddle");
        } else if (other.group == "brick") {

            if (global["activeBroken"] == 0) {
                bounce_y(ball);
            }

            kill(other);
            global["score"] += 100;
            play_sound(ball, "paddle");
            global["briks"] -= 1;

            if (global["briks"] <= 0) {
                kill(ball);
            } else {
                try_spawn_powerup(ball);
            }
        }
    }
}

function try_spawn_powerup(ball) {
    
    if (probability(15)) {

        let p = parseInt(random(0, 4));

        if (p == 0) spawn(ball, "gun");
        if (p == 1) spawn(ball, "big");
        if (p == 2) spawn(ball, "small");
       // if (p == 3) spawn(ball, "shoot");
        if (p == 4) spawn(ball, "broken");
    }
}