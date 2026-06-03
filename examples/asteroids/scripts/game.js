/// <reference path="../../../../tools/scripts/flx.d.ts" />

/*
    Defines the initial game state.
*/

// CONSTANTS
const INIT_LIVES = 3;

function start() {
    global["score"] = 0;
    global["lives"] = INIT_LIVES;
    global["asteroids"] = 0;
    global["inGame"] = 0;
    global["shipDead"] = 0;
}
