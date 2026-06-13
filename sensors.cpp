#include "sensors.h"

#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <SensirionI2cSps30.h>

#define SDA_PIN 20
#define SCL_PIN 19

static SCD30 scd30;
static SensirionI2cSps30 sps30;

static float co2 = 0;
static float temp_scd = 0;
static float hum_scd = 0;
static float pm2_5 = 0;
static float pm10 = 0;
static float temp_offset = 0;
static float hum_offset = 0;

static void initSCD30();
static void initSPS30();
static void readSCD30();
static void readSPS30();

void initSensors()
{
  Wire.begin(SDA_PIN, SCL_PIN);

  initSCD30();
  initSPS30();
}

static void initSCD30()
{
  if (!scd30.begin(Wire))
  {
    Serial.println("SCD30 not found");
    return;
  }
}

static void initSPS30()
{
  sps30.begin(Wire, SPS30_I2C_ADDR_69);

  if (sps30.stopMeasurement() != 0)
  {
    // Sensor may already be idle; ignore the error.
  }

  if (sps30.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_FLOAT) != 0)
  {
    Serial.println("SPS30 start failed");
  }
}

static void readSCD30()
{
  if (scd30.readMeasurement())
  {
    co2 = scd30.getCO2();
    temp_scd = scd30.getTemperature();
    hum_scd = scd30.getHumidity();
  }
}

static void readSPS30()
{
  uint16_t dataReadyFlag = 0;
  int16_t err = sps30.readDataReadyFlag(dataReadyFlag);

  if (err != 0 || dataReadyFlag == 0)
    return;

  float mc1p0 = 0, mc2p5 = 0, mc4p0 = 0, mc10p0 = 0;
  float nc0p5 = 0, nc1p0 = 0, nc2p5 = 0, nc4p0 = 0, nc10p0 = 0;
  float typicalParticleSize = 0;

  int16_t readErr = sps30.readMeasurementValuesFloat(
      mc1p0, mc2p5, mc4p0, mc10p0,
      nc0p5, nc1p0, nc2p5, nc4p0, nc10p0,
      typicalParticleSize);

  if (readErr == 0)
  {
    pm2_5 = mc2p5;
    pm10 = mc10p0;
  }
}

void updateSensors()
{
  readSCD30();
  readSPS30();
}

void setTempOffset(float val)
{
  temp_offset = val;
}

void setHumidityOffset(float val)
{
  hum_offset = val;
}

float getCO2() { return co2; }
float getPM25() { return pm2_5; }
float getPM10() { return pm10; }
float getTempLocal() { return temp_scd + temp_offset; }
float getHumidityLocal() { return hum_scd + hum_offset; }
