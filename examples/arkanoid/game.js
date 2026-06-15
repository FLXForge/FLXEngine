/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

let level = 0;

function born(game){
	fade_set(1);
	fade_off()
}

function action(game){
	if (level == 0 && Key.pressed(KEY_SPACE)){
		fade_on();
		game.local["transition"] = 1;
		level = 1;
	}

	if (fade_done() && game.local["transition"] == 1) {
		keep_only(game);
		spawn(game, "level1");
		game.local["transition"] = 0;
	}
}