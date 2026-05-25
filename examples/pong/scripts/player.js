/// <reference path="../../../../scripting tools/flx.d.ts" />

/* 
    The player moves using the up and down keys. 
*/

function action(self) {
    if (Key.up()) {
        move_y(self, UP);
    }

    if (Key.down()) {
        move_y(self, DOWN);
    }
}