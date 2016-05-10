# Weather and Greenhouse Data Server
This is a simple weather and greenhouse data server/logger daemon

### config.json

To run the app you will need to configure it for your setup. The following is a sample config.json
```
{
        "open_weather_map_apikey"=""
        "latitude"="37.3230"
        "longitude"="-122.0322"
        "sqlite3_file"="/home/pi/data/weather_data_pi.sqlite"
}
```

`open_weather_map_apikey` You will need to create a free account at openweathermap.org and create an API key which you will substitute here.

The `latitude` and `longitude` values for the location you wish to retrieve current weather and weather forecasting for.

`sqlite3_file` is the location where you want your sqlite file to be stored on disk


### Running

After building simply run
```
$> ./greenhouse_data_server config.json
```
Providing the path to your `config.json` file