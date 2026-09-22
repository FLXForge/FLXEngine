#include "engine/cli/CliApplication.h"

int main(int argc, char* argv[])
{
    CliApplication application;
    return application.run(argc, argv);
}
