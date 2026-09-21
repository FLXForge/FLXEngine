/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function action(sensor) {
    const hit = ray(sensor, sensor.angle, 180);

    if (hit === undefined) {
        return;
    }

    if (hit.object.group === "enemy") {
        // Turn away after the first successful query.
        // ray() did not need with[] and did not dispatch collision().
        apply_angle(sensor, 270);
    }
}
