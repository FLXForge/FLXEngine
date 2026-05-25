/// <reference path="../../../../scripting tools/flx.d.ts" />

/* 
	The enemy paddle always follows the ball on the y-axis 
*/

function motion(self) {
	follow_y(self, "ball");
}