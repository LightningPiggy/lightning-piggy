This converts https://github.com/nayarsystems/posix_tz_db/raw/refs/heads/master/zones.csv into C code, which is then included in the build.

Usage:

```
wget https://github.com/nayarsystems/posix_tz_db/raw/refs/heads/master/zones.csv
python3 csv_to_h.py # should generate timezones.h
```
