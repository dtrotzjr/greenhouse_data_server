# Weather and Greenhouse Data Server
This is a simple weather and greenhouse data server/logger daemon

## Installation

The installation steps assume you are running the Raspian OS on your Raspberry Pi. If you are running another OS adjust the installation accordingly.

### Requirements
A Raspberry Pi is recommended but not required for this project. You can use any computer running a modern Linux OS.

`libsqlite3-dev` is required for data storage. To install on your Raspberry pi run the command:
```
$> sudo apt-get install libsqlite3-dev
```

`libjsoncpp-dev` is required to parse the data stream from OpenWeatherMap.org's API calls. To install on your Raspberry pi run the command:
```
$> sudo apt-get install libjsoncpp-dev
```

OPTIONAL: If you want to view the sqlite database from the commandline be install the sqlite3 package
```
$> sudo apt-get install sqlite3
```

## Running

### Create a config.json file

To run the app you will need to configure it for your setup. The following is a sample config.json
```
{
    "open_weather_map_apikey":"<YOUR_OPEN_WEATHERMAP_APIKEY>",
    "wunderground_apikey":"<YOUR_WUNDERGROUND_APIKEY>",
    "forcast_io_api_key":"<YOUR_FORECAST_IO_APIKEY>",
    "latitude":"37.3230",
    "longitude":"-122.0322",
    "sqlite3_file":"/home/pi/local/greenhouse_server/data/weather_data_pi.sqlite"
}
```

`open_weather_map_apikey` You will need to create a free account at openweathermap.org and create an API key which you will substitute here.

The `latitude` and `longitude` values for the location you wish to retrieve current weather and weather forecasting for.

`sqlite3_file` is the location where you want your sqlite file to be stored on disk


### Run the app

After building simply run. Be sure to provide the path to your `config.json` file
```
$> ./greenhouse_data_server /home/pi/local/greenhouse_server/config/config.json
```
