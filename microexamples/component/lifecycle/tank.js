function born(object)
{
    play_timer(object, "destroy", 4);
}

function action(object)
{
    if (timer_done(object, "destroy"))
    {
        kill(object);
    }
}

function draw(object)
{
    draw_text(320, 30, "COMPONENT LIFECYCLE", 10);
    draw_text(320, 50, "tank + turret + cannon = same entity", 8);
    draw_text(320, 65, "projectile = normal child", 8);
    draw_text(320, 90, "tank dies after 4 seconds", 8);
}