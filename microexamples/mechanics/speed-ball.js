/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function born(ball)
{
    write_local(ball, "elapsed", 0);
    write_local(ball, "fast", 0);
}

function action(ball)
{
    write_local(ball, "elapsed", read_local(ball, "elapsed") + delta());

    if (read_local(ball, "fast") == 0 && read_local(ball, "elapsed") >= 2.0) {
        apply_speed(ball, 80);
        write_local(ball, "fast", 1);
        write_local(ball, "elapsed", 0);
    }
    else if (read_local(ball, "fast") == 1 && read_local(ball, "elapsed") >= 2.0) {
        restore_speed(ball);
        write_local(ball, "fast", 0);
        write_local(ball, "elapsed", 0);
    }
}

function motion(ball)
{
    advance(ball);

    if (ball.x >= 300 || ball.x <= 20) {
        reflect_x(ball);
    }
}
