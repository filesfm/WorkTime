#pragma once

class OneInstanceGuarantor
{
public:
    static void createPidFile();
    static void deletePidFile();
};
