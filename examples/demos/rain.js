/*
    Version A

function draw()
{
    for(let i=0;i<180;i++)
    {
        let x = random(0, 640);
        let y = random(0, 480);

        draw_line(
            x,
            y,
            x-3,
            y+10,
            "gray"
        );
    }
}
*/

/*
    Version B
*/

let offset = 0;

function draw()
{
    offset += 6;

    if(offset>480)
        offset = 0;

    for(let i=0;i<200;i++)
    {
        let x = random(0, 640);

        let y =
            (random(0, 480) + offset)
            % 480;

        draw_line(
            x,
            y,
            x-2,
            y+8,
            "gray"
        );
    }
}