import csv

# https://github.com/nayarsystems/posix_tz_db/raw/refs/heads/master/zones.csv

# Input and output filenames
csv_filename = "zones.csv"
header_filename = "timezones.h"

# Read CSV and generate C++ code
with open(csv_filename, "r") as csv_file:
    reader = csv.reader(csv_file)
    timezones = list(reader)

# Generate the header file
with open(header_filename, "w") as header_file:
    header_file.write("#ifndef TIMEZONES_H\n#define TIMEZONES_H\n\n")
    header_file.write("#include <Arduino.h>\n#include <pgmspace.h>\n\n")

    header_file.write("struct TimeZoneMapping {\n    const char* city;\n    const char* posixTZ;\n};\n\n")

    header_file.write("const TimeZoneMapping timeZoneMap[] PROGMEM = {\n")
    for city, tz in timezones:
        header_file.write(f'    {{"{city}", "{tz}"}},\n')
    header_file.write("};\n\n")

    header_file.write("#define TIMEZONE_COUNT (sizeof(timeZoneMap) / sizeof(timeZoneMap[0]))\n\n")
    header_file.write("#endif // TIMEZONES_H\n")

print(f"Generated {header_filename} successfully!")

