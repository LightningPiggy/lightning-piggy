Lightning Piggy Web Config
==========================

How it gets integrated into the build:
---------------------------------------

- input.css is converted into output_tailwind.css

```
~/software/tailwindcss-linux-x64-v3.4.17 -i input.css -o tailwind_output.css --minify
```

- output_tailwind.css is included into index.html to reduce the number of HTTP-calls, resulting in the file: index_inlined.html

```
python3 inline_css.py
```

- index_inlined.html is gzip'ped and converted to index.html.gzip.h which defines: const unsigned char index_gzip[] PROGMEM 

```
python3 gzip_file_to_PROGMEM_header.py index_inlined.html index.html.gzip.h
```

- index.html.gzip.h is compiled into the Arduino build
- the Arduino build uses index_gzip to reply to the HTTP-call for the path /




How to develop:
---------------
Run:

~/software/tailwindcss-linux-x64-v3.4.17 -i input.css -o tailwind_output.css --watch

Also run:

python3 server.py

Then browse to:

http://localhost:5000/

Or to test the inlined version, browse to:

http://localhost:5000/index_inlined.html
