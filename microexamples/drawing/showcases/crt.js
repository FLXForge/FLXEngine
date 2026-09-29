function draw()
{
    for(let y=0;y<480;y+=2)
    {
        draw_line(
            0,
            y,
            640-1,
            y,
            "#202020"
        );
    }

    for(let i=0;i<1200;i++)
    {
        draw_pixel(
            random(0, 640),
            random(0, 480),
            "white"
        );
    }
}