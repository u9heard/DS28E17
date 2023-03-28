#include <Arduino.h>
#include <OneWire.h>
#include <DS28E17.cpp>

OneWire ow(5);
DS28E17Addr device;
DS28E17 ds(&ow);

void printAddress(DS28E17Addr deviceAddress)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		if (deviceAddress[i] < 16) Serial.print("0");
		Serial.print(deviceAddress[i], HEX); 
	}
	Serial.println();
}

void setup() {
  Serial.begin(9600);
  ow.search(device);
  printAddress(device);
  ds.setAddress(device);
}

void loop() {
  ds.UpdateData(SHT_READ_HIGH);

  Serial.println(ds.getTemperature());
	Serial.println(ds.getHumidity());
  Serial.println("--------------");

  delay(1000);
}