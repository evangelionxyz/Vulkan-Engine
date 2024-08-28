// Copyright 2024, Evangelion Manuhutu
#include "core/logger.h"
#include "core/application.h"

int main(i32 argc, char** argv)
{
    const Logger *logger = new Logger();
    Application application(argc, argv);
    application.run();
    delete logger;
}
