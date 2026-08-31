/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/* 
	The enemy paddle always follows the ball on the y-axis 
*/

function motion(enemy) {
    const balls = find_name("ball");

    if (balls.length > 0) {
        follow_y(enemy, balls[0]);
    }
}
