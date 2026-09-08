function action(sensor) {
    const hit = ray(sensor, sensor.angle, 120);

    if (!hit) {
        return;
    }

    if (hit.object.group === "enemy") {
        write_local(sensor, "seen", hit.object.id);
    }
}
