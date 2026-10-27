// Production consumer: links ONLY flx-core, with no FLX_TESTING or CLI dependency.
#include <core/FlxSession.h>
#include "SessionTestProject.h"
#include <iostream>
int main()
{
    FlxSessionOptions options;
    options.enableVideo = options.enableAudio = false;
    FlxSession session;
    for (int execution = 0; execution < 2; ++execution)
    {
        if (!session.load(sessionTestProject("let n=0;function action(o){if(++n===2)exit();}"), options)) return 1;
        if (!session.update(0.1f) || session.finished()) return 2;
        if (!session.update(0.1f) || !session.finished()) return 3;
        if (session.result().exitReason != FlxSessionExitReason::ScriptRequestedExit || session.result().framesExecuted != 1) return 4;
        session.unload();
    }
    std::cout << "[PASS] independent production consumer of flx-core\n";
}
