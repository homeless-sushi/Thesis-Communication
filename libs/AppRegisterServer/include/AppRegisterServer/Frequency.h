#ifndef APP_REGISTER_SERVER_FREQUENCY
#define APP_REGISTER_SERVER_FREQUENCY

namespace Frequency
{
    enum FrequencyStep
    {
        FREQUENCY_LVL_1 = 1400000, 
        FREQUENCY_LVL_2 = 1700000,
        FREQUENCY_LVL_3 = 2900000,
        N_FREQUENCY_STEPS = 3
    };

    FrequencyStep GetCurrentFrequency(unsigned int cpu);
    FrequencyStep GetNextFrequency(FrequencyStep freq);
    void SetFrequency(unsigned int cpu, FrequencyStep freq);
}

#endif //APP_REGISTER_SERVER_FREQUENCY
