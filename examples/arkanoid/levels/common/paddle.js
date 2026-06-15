/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function action(paddle){
	if (Key.down(KEY_LEFT)) {
        move_x(paddle, LEFT);
    }

    if (Key.down(KEY_RIGHT)) {
        move_x(paddle, RIGHT);
    }
}

function collision(paddle, wall) {

    if (wall.group == "wall_side") {

        if (paddle.x < wall.x) {
            paddle.x = wall.x - paddle.width;
        } else {
            paddle.x = wall.x + wall.width;
        }
    }
}