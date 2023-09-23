#include "AppRegisterServer/Sensors.h"

#include <string>

namespace Sensors
{
    void Sensors::readSensors()
    {
        currSocW = readSensor(SENSOR_IN_W);
        currSocA = readSensor(SENSOR_IN_A);
        currSocV = readSensor(SENSOR_IN_V);
        currCpuW = readSensor(SENSOR_CPU_W);
        currCpuA = readSensor(SENSOR_CPU_A);
        currCpuV = readSensor(SENSOR_CPU_V);
        currGpuW = readSensor(SENSOR_GPU_W);
        currGpuA = readSensor(SENSOR_GPU_A);
        currGpuV = readSensor(SENSOR_GPU_V);
    };

    float Sensors::readSensor(std::string sensorPath)
    {
        float value;
        FILE* fp;
        fp = fopen(sensorPath.c_str(), "r");
        if(fp) {
            fscanf(fp, "%f", &value);
            fclose(fp);
        } else {
            value = -1;
        }
        return value;
    };
}
