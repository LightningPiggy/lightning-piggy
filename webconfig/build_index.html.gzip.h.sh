~/software/tailwindcss-linux-x64-v3.4.17 -i input.css -o tailwind_output.css --minify
python3 inline_css.py
python3 gzip_file_to_PROGMEM_header.py index_inlined.html index.html.gzip.h

