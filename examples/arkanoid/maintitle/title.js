/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function motion(title) {
    if (title.local["hit"] == 1) {
        title.local["hitTime"] += delta();

        let t = title.local["hitTime"];

        title.y = title.originY - Math.sin(t * 20) * 6 * (1 - t);

        if (t >= 1) {
            title.local["hit"] = 0;
            title.local["hitTime"] = 0;
            title.y = title.originY;
        }
    }
}