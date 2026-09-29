/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

const RAY_DISTANCE = 180;
const ROTATION_SPEED = 60;

function action(sensor) {
    rotate(sensor, 1);

    const hit = ray(sensor, sensor.angle, 180);

    if (hit === undefined) {
        restore_color(sensor);
        return;
    }

    apply_color(sensor, "green");
}