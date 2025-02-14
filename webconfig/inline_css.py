import re

# Define file paths
html_file = "index.html"
out_file = "index_inlined.html"
css_file = "tailwind_output.css"

# Read the CSS content
with open(css_file, "r", encoding="utf-8") as f:
    css_content = f.read()

# Read the HTML file
with open(html_file, "r", encoding="utf-8") as f:
    html_content = f.read()

# Replace the <link> tag with an inline <style> tag
inline_style = f"<style>{css_content}</style>"
html_content = re.sub(r'<link\s+rel="stylesheet"\s+href="tailwind_output\.css">', inline_style, html_content)

# Write the updated HTML file
with open(out_file, "w", encoding="utf-8") as f:
    f.write(html_content)

print("Replaced external Tailwind CSS with inline styles!")
