import gzip
import os
import sys

def gzip_to_header(input_file, output_header):
    # Read and compress the input file
    with open(input_file, 'rb') as f:
        compressed_data = gzip.compress(f.read())

    # Extract filename without extension for variable naming
    var_name = "index_gzip"

    # Define line break size (e.g., 16 bytes per line for readability)
    bytes_per_line = 16
    array_lines = []
    for i in range(0, len(compressed_data), bytes_per_line):
        line = ', '.join(f'0x{byte:02X}' for byte in compressed_data[i:i + bytes_per_line])
        array_lines.append("    " + line)  # Indent for readability
    
    # Generate the header file content without using f-strings with backslashes
    header_content = (
        "#ifndef " + var_name.upper() + "_H\n"
        "#define " + var_name.upper() + "_H\n\n"
        "#include <Arduino.h>\n"
        "#include <pgmspace.h>\n\n"
        "const unsigned char " + var_name + "[] PROGMEM = {\n"
        + ",\n".join(array_lines) + "\n"
        "};\n\n"
        "const unsigned int " + var_name + "_length = " + str(len(compressed_data)) + ";\n\n"
        "#endif // " + var_name.upper() + "_H\n"
    )

    # Write to output .h file
    with open(output_header, 'w') as f:
        f.write(header_content)

    print("Generated " + output_header + " from " + input_file + " (" + str(len(compressed_data)) + " bytes compressed)")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 gzip_to_header.py <input_file> <output_header>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_header = sys.argv[2]

    gzip_to_header(input_file, output_header)

