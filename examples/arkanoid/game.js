/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

const INIT_LIVES = 3;
const MAX_LEVEL = 4;

let currentLevel = 0;
let nextLevel = 0;

function born(game){
    reset_game();

    fade_set(1);
    fade_off();
}

function action(game){

    handle_title_start(game);
    handle_level_completed(game);
    handle_game_over(game);
    handle_return_to_title(game);
    handle_transition(game);
    draw_hud();
}

function reset_game(){
    global["lives"] = INIT_LIVES;
    global["score"] = 0;
    global["show_hud"] = 0;
    global["game_over"] = 0;
    global["congratulations"] = 0;
    global["briks"] = 0;

    currentLevel = 0;
    nextLevel = 0;
}

function handle_title_start(game){

    if (currentLevel == 0 && Key.pressed(KEY_SPACE)){
        nextLevel = 1;
        game.local["transition"] = 1;
        fade_on();
    }
}

function handle_level_completed(game){

    if (currentLevel < 1){
        return;
    }

    if (global["briks"] > 0){
        return;
    }

    if (game.local["transition"] == 1){
        return;
    }

    if (currentLevel < MAX_LEVEL){
        nextLevel = currentLevel + 1;
        game.local["transition"] = 1;
        global["show_hud"] = 0;
        fade_on();
    } else {
        global["show_hud"] = 0;
        keep_only(game);
        spawn(game, "congratulations");

        currentLevel = 0;
        nextLevel = 0;

        fade_set(1);
        fade_off();
    }
}

function handle_game_over(game){

    if (global["game_over"] != 1){
        return;
    }

    global["game_over"] = 2;
    global["show_hud"] = 0;

    keep_only(game);
    spawn(game, "gameover");

    currentLevel = 0;
    nextLevel = 0;

    fade_set(1);
    fade_off();
}

function handle_return_to_title(game){

    if (
        global["game_over"] != 3
        && global["congratulations"] != 2
    ){
        return;
    }

    keep_only(game);
    reset_game();

    spawn(game, "title");

    fade_set(1);
    fade_off();
}

function handle_transition(game){

    if (game.local["transition"] != 1){
        return;
    }

    if (!fade_done()){
        return;
    }

    keep_only(game);

    if (nextLevel == 1){
        spawn(game, "level1");
        global["briks"] = 6 * 13;
    }

    if (nextLevel == 2){
        spawn(game, "level2");
        global["briks"] = 6 * 13;
    }

    if (nextLevel == 3){
        spawn(game, "level3");
        global["briks"] = 6 * 13;
    }

    if (nextLevel == 4){
        spawn(game, "level4");
        global["briks"] = 6 * 13;
    }

    currentLevel = nextLevel;
    nextLevel = 0;

    game.local["transition"] = 0;

    fade_set(1);
    fade_off();
}

function draw_hud(){

    if (global["show_hud"] != 1){
        return;
    }

    let lives = "";

    for(let i = 0; i < global["lives"]; i++){
        lives += " o";
    }

    draw_text(
        40,
        300,
        "SCORE: " + global["score"],
        16,
        "white"
    );

    draw_text(
        500,
        300,
        "LIVES: " + lives,
        16,
        "white"
    );
}