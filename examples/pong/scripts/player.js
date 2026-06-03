/// <reference path="../../../../tools/scripts/flx.d.ts" />

/* 
    The player moves using the up and down keys. 
*/

function action(self) {
    if (Key.down(KEY_UP)) {
        move_y(self, UP);
    }

    if (Key.down(KEY_DOWN)) {
        move_y(self, DOWN);
    }
}