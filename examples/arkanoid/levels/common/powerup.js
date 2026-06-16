/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function motion(powerup){
	move_y(powerup, DOWN);

	if (powerup.y > 320) {
		kill(powerup);
	}
}