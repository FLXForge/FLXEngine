/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function motion(ball){
	advance(ball);
}

function collision(ball, other) {
    if (other.group == "wall_top") {
        reflect_y(ball);
        play_sound(ball, "paddle");
    } else if (other.group == "wall_side") {
        reflect_x(ball);
        play_sound(ball, "paddle");
    } else if (other.group == "paddle") {
        reflect_y(ball);
        play_sound(ball, "paddle");
    } else if (other.group == "title") {
        other.local["hit"] = 1;
        other.local["hitTime"] = 0;
    }
}
