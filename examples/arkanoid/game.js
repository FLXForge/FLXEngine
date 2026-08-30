/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

const INIT_LIVES = 3;
const MAX_LEVEL = 4;
const FIRE_BUTTON = button(0);

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
    write_global("level", currentLevel);
}

function reset_game(){
    write_global("lives", INIT_LIVES);
    write_global("score", 0);
    write_global("show_hud", 0);
    write_global("game_over", 0);
    write_global("congratulations", 0);
    write_global("briks", 0);

    currentLevel = 0;
    nextLevel = 0;
}

function handle_title_start(game){

    if (currentLevel == 0 && input_pressed(game, FIRE_BUTTON)){
        nextLevel = 1;
        write_local(game, "transition", 1);
        fade_on();
    }
}

function handle_level_completed(game){

    if (currentLevel < 1){
        return;
    }

    if (read_global("briks") > 0){
        return;
    }

    if (read_local(game, "transition") == 1){
        return;
    }

    if (currentLevel < MAX_LEVEL){
        nextLevel = currentLevel + 1;
        write_local(game, "transition", 1);
        write_global("show_hud", 0);
        fade_on();
    } else {
        write_global("show_hud", 0);
        keep_only(game);
        spawn(game, "congratulations");

        currentLevel = 0;
        nextLevel = 0;

        fade_set(1);
        fade_off();
    }
}

function handle_game_over(game){

    if (read_global("game_over") != 1){
        return;
    }

    write_global("game_over", 2);
    write_global("show_hud", 0);

    keep_only(game);
    spawn(game, "gameover");

    currentLevel = 0;
    nextLevel = 0;

    fade_set(1);
    fade_off();
}

function handle_return_to_title(game){

    if (
        read_global("game_over") != 3
        && read_global("congratulations") != 2
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

    if (read_local(game, "transition") != 1){
        return;
    }

    if (!fade_done()){
        return;
    }

    keep_only(game);

    if (nextLevel == 1){
        spawn(game, "level1");
        write_global("briks", 39);
    }

    if (nextLevel == 2){
        spawn(game, "level2");
        write_global("briks", 65);
    }

    if (nextLevel == 3){
        spawn(game, "level3");
        write_global("briks", 59);
    }

    if (nextLevel == 4){
        spawn(game, "level4");
        write_global("briks", 104);
    }

    currentLevel = nextLevel;
    nextLevel = 0;

    write_local(game, "transition", 0);

    fade_set(1);
    fade_off();
}

function draw_hud(){

    if (read_global("show_hud") != 1){
        return;
    }

    let lives = "";

    for(let i = 0; i < read_global("lives"); i++){
        lives += " o";
    }

    draw_text(
        40,
        300,
        "SCORE: " + read_global("score"),
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
