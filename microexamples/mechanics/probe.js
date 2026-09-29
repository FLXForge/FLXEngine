/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function motion(probe)
{
    advance(probe);

    if (probe.x < -20 || probe.x > 340 || probe.y < -20 || probe.y > 260) {
        kill(probe);
    }
}
