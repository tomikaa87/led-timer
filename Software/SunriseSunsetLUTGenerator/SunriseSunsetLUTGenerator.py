import argparse
import datetime
import datetime as dt
from pytz import timezone
from skyfield import almanac
from skyfield.api import wgs84, load


def calculate(latitude, longitude, year, day_of_year):
    zone = timezone('UTC')
    d = dt.datetime(year=year, month=1, day=1)
    d += datetime.timedelta(days=day_of_year)
    now = zone.localize(d)
    midnight = now.replace(hour=0, minute=0, second=0, microsecond=0)
    next_midnight = midnight + dt.timedelta(days=1)

    ts = load.timescale()
    t0 = ts.from_datetime(midnight)
    t1 = ts.from_datetime(next_midnight)
    eph = load('de421.bsp')
    position = wgs84.latlon(latitude, longitude)
    f = almanac.dark_twilight_day(eph, position)
    times, events = almanac.find_discrete(t0, t1, f)

    day_start_index = 0
    for e in events:
        if e == 4:  # look for "Day" event
            break
        day_start_index += 1

    sunrise = (times[day_start_index].utc.hour, times[day_start_index].utc.minute)
    sunset = (times[day_start_index + 1].utc.hour, times[day_start_index + 1].utc.minute)

    return sunrise, sunset


def make_lut(latitude: float, longitude: float):
    print('#include "Clock.h"')
    print('#include <stdint.h>')
    print(f'// Sunrise-sunset LUT for latitude={latitude}, longitude={longitude}.')
    print('// Made by SunriseSunsetLUTGenerator.py.')
    print('// First value is the sunrise, second is the sunset, both measured in minutes from midnight, in UTC.')
    print('// During non-leap years, February 29th (index=59) must be skipped, all subsequent indices must be offset by 1.')
    print('const uint8_t SunriseSunsetLUT[366][2] = {\n    ', end='')
    column = 0
    for doy in range(0, 366):
        sunrise, sunset = calculate(latitude, longitude, 2024, doy)
        sunrise_buckets_from_midnight = int((sunrise[0] * 60 + sunrise[1]) / 6)
        sunset_buckets_from_midnight = int((sunset[0] * 60 + sunset[1]) / 6)
        assert(0 <= sunrise_buckets_from_midnight <= 255)
        assert(0 <= sunset_buckets_from_midnight <= 255)
        print(f'/* Day {doy + 1:03d} */ {{ 0x{sunrise_buckets_from_midnight:02X}, 0x{sunset_buckets_from_midnight:02X} }}', end='')
        column += 1
        delimiter = ',' if doy < 365 else ''
        print(delimiter, end='')
        if doy < 365:
            if column == 2:
                column = 0
                print('\n    ', end='')
            else:
                print(' ', end='')
        # print(f'DoY={doy:3d}, Sunrise={sunrise}, Sunset={sunset}')
    print('\n};')


argparser = argparse.ArgumentParser()
argparser.add_help = True

argparser.add_argument('--latitude', required=True, help='Latitude in degrees, positive for North, negative for South')
argparser.add_argument('--longitude', required=True, help='Longitude in degrees, positive for East, negative for West')

args = argparser.parse_args()

if args.latitude is None:
    print('--latitude is mandatory')
    exit(1)

if args.longitude is None:
    print('--longitude is mandatory')
    exit(1)

try:
    make_lut(float(args.latitude), float(args.longitude))
except ValueError:
    print('Latitude and longitude must be valid decimal numbers')
    exit(1)
