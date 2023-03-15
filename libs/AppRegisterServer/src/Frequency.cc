#include "AppRegisterServer/Frequency.h"

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <stdio.h>

namespace Frequency
{
    FrequencyStep GetCurrentFrequency(unsigned int cpu)
    {
        std::string cmd("sudo cpufreq-info -w -c " + std::to_string(cpu));

        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe) {
            std::cerr << "ERROR: While calling popen()" << std::endl;
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }

        return FrequencyStep(std::stoul(result.c_str()));
    };


    FrequencyStep GetNextFrequency(FrequencyStep freq)
    {
        const FrequencyStep frequencySteps[] = {
            FREQUENCY_LVL_1, 
            FREQUENCY_LVL_2,
            FREQUENCY_LVL_3
        };

        unsigned int i = 0;
        while(frequencySteps[i] != freq && i < N_FREQUENCY_STEPS)
            i++;

        return frequencySteps[(i+1)%N_FREQUENCY_STEPS];
    }

    void SetFrequency(unsigned int cpu, FrequencyStep freq)
    {
        std::string cmd1("sudo cpufreq-set -g userspace -c " + std::to_string(cpu));
        std::string cmd2("sudo cpufreq-set -c " + std::to_string(cpu) + " -f " + std::to_string(freq));

        system(cmd1.c_str());
        system(cmd2.c_str());
    };
}
