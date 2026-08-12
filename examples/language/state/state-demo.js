function action(demo)
{
    if (state_entered(demo)) {
        console.log("Entered state: " + state_current(demo));
    }

    if (state_active(demo, "idle") && state_time(demo) >= 2.0) {
        state_to(demo, "warning");
    }

    if (state_active(demo, "warning") && state_time(demo) >= 2.0) {
        state_to(demo, "done");
    }
}

function draw(demo)
{
    var color = "white";

    if (state_active(demo, "idle")) {
        color = "green";
    } else if (state_active(demo, "warning")) {
        color = "yellow";
    } else if (state_active(demo, "done")) {
        color = "red";
    }

    draw_text(20, 30, "FLX State Demo", 32, "white");
    draw_text(20, 80, "State: " + state_current(demo), 24, color);
    draw_text(20, 115, "Time: " + state_time(demo).toFixed(2), 24, color);

    if (state_entered(demo)) {
        draw_text(20, 155, "ENTERED", 20, color);
    }
}
