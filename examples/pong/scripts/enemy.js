/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/* 
	The enemy paddle always follows the ball on the y-axis 
*/

function motion(enemy) {
	follow_y(enemy, "ball");
}