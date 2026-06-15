#pragma once

void initSensors();
void updateSensors();

float getCO2();
float getPM25();
float getPM10();
float getTempLocal();
float getHumidityLocal();
void calibrateCO2(float val);
void setTempOffset(float val);
void setHumidityOffset(float val);