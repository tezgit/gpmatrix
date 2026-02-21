#!/usr/bin/env python3
"""
PNG to 8x8 LED Matrix Converter for RP2040 Icon Display
Converts square PNG images to C++ array format for use with Adafruit NeoPixel
"""

from PIL import Image
import numpy as np
import os
import sys

def resize_to_8x8(image_path, output_size=(8, 8)):
    """
    Resize an image to 8x8 pixels using high-quality downsampling
    """
    try:
        # Open the image
        img = Image.open(image_path)
        
        # Convert to RGB if necessary
        if img.mode != 'RGB':
            img = img.convert('RGB')
        
        # Check if square (optional warning)
        if img.width != img.height:
            print(f"⚠️  Warning: Image is not square ({img.width}x{img.height}). Will be stretched to 8x8.")
        
        # Resize to 8x8 using LANCZOS resampling (high quality)
        img_resized = img.resize(output_size, Image.Resampling.LANCZOS)
        
        return img_resized
    
    except Exception as e:
        print(f"❌ Error opening image: {e}")
        sys.exit(1)

def image_to_color_matrix(img):
    """
    Convert PIL Image to 8x8x3 numpy array of RGB values
    """
    # Convert to numpy array
    img_array = np.array(img)
    
    # Ensure shape is (8, 8, 3)
    if img_array.shape != (8, 8, 3):
        print(f"❌ Error: Expected 8x8x3 array, got {img_array.shape}")
        sys.exit(1)
    
    return img_array

def apply_brightness_scaling(color_matrix, scale_to_255=True):
    """
    Scale color values to 0-255 range if needed
    Also can apply brightness scaling here
    """
    if scale_to_255:
        # Find max value to scale to 0-255 if image uses different range
        max_val = np.max(color_matrix)
        if max_val > 0 and max_val != 255:
            color_matrix = (color_matrix / max_val * 255).astype(np.uint8)
    
    return color_matrix

def generate_c_array(color_matrix, icon_name="icon", output_file=None):
    """
    Generate C++ array code from color matrix
    """
    height, width, channels = color_matrix.shape
    
    # Start building the C array string
    c_array = f"// Auto-generated icon: {icon_name}\n"
    c_array += f"const uint8_t icon_{icon_name}[{height}][{width}][3] = {{\n"
    
    for y in range(height):
        c_array += "    {"
        for x in range(width):
            r, g, b = color_matrix[y, x]
            c_array += f"{{{r},{g},{b}}}"
            if x < width - 1:
                c_array += ", "
        c_array += "}"
        if y < height - 1:
            c_array += ",\n"
        else:
            c_array += "\n"
    
    c_array += "};\n\n"
    
    # Also generate a 1D flat version (alternative format)
    c_array += f"// Flat 1D version (192 bytes)\n"
    c_array += f"const uint8_t icon_{icon_name}_flat[192] = {{\n    "
    
    flat_values = []
    for y in range(height):
        for x in range(width):
            r, g, b = color_matrix[y, x]
            flat_values.extend([str(r), str(g), str(b)])
    
    # Format in rows of 12 values for readability
    for i in range(0, len(flat_values), 12):
        chunk = flat_values[i:i+12]
        c_array += ", ".join(chunk)
        if i + 12 < len(flat_values):
            c_array += ",\n    "
    
    c_array += "\n};\n"
    
    if output_file:
        with open(output_file, 'w') as f:
            f.write(c_array)
        print(f"✅ Array saved to {output_file}")
    
    return c_array

def generate_icons_h(icon_names, output_file="icons.h"):
    """
    Generate a complete icons.h file with multiple icons
    """
    header = "// Auto-generated icon arrays for 8x8 LED matrix\n"
    header += "#ifndef ICONS_H\n"
    header += "#define ICONS_H\n\n"
    header += "#include <stdint.h>\n\n"
    
    # We'll collect individual array contents
    # This assumes you have separate .h files or arrays for each icon
    # For this example, we'll create a template
    
    footer = "// Icon lookup table\n"
    footer += "const uint8_t* const icon_table[] = {\n"
    
    for i, name in enumerate(icon_names):
        footer += f"    (const uint8_t*)icon_{name},\n"
    
    footer += "};\n\n"
    footer += f"#define NUM_ICONS {len(icon_names)}\n\n"
    footer += "#endif // ICONS_H\n"
    
    with open(output_file, 'w') as f:
        f.write(header + footer)
    
    print(f"✅ icons.h generated with {len(icon_names)} icons")

def preview_ascii(color_matrix):
    """
    Print an ASCII preview of the icon (grayscale approximation)
    """
    chars = " .:-=+*#%@"
    print("\n📺 ASCII Preview:")
    for y in range(8):
        line = ""
        for x in range(8):
            r, g, b = color_matrix[y, x]
            # Convert to grayscale intensity
            gray = int(0.299 * r + 0.587 * g + 0.114 * b)
            # Map to character
            char_index = int(gray / 255 * (len(chars) - 1))
            line += chars[char_index] * 2  # Double for better visibility
        print(line)
    print()

def batch_convert(folder_path, output_file="icons.h"):
    """
    Convert all PNG images in a folder to icons
    """
    icon_arrays = []
    icon_names = []
    
    # Get all PNG files in folder
    png_files = sorted([f for f in os.listdir(folder_path) if f.lower().endswith('.png')])
    
    if not png_files:
        print(f"❌ No PNG files found in {folder_path}")
        return
    
    print(f"🔄 Found {len(png_files)} PNG files")
    
    for i, png_file in enumerate(png_files):
        # Create icon name from filename (without extension)
        base_name = os.path.splitext(png_file)[0]
        # Sanitize name for C identifier
        icon_name = base_name.replace(' ', '_').replace('-', '_')
        icon_name = f"{i:02d}_{icon_name}"  # Add index prefix
        
        print(f"\n🔄 Processing {png_file} -> {icon_name}")
        
        # Process image
        img_path = os.path.join(folder_path, png_file)
        img = resize_to_8x8(img_path)
        color_matrix = image_to_color_matrix(img)
        color_matrix = apply_brightness_scaling(color_matrix)
        
        # Generate individual array file
        array_code = generate_c_array(color_matrix, icon_name, f"{icon_name}.h")
        
        # Preview
        preview_ascii(color_matrix)
        
        icon_arrays.append(array_code)
        icon_names.append(icon_name)
    
    # Generate master icons.h
    generate_icons_h(icon_names, output_file)
    
    print(f"\n✅ Batch conversion complete! Generated {len(icon_names)} icons")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Convert PNG images to 8x8 LED matrix C arrays')
    parser.add_argument('input', help='Input PNG file or folder path')
    parser.add_argument('-o', '--output', default='icon_output.h', 
                       help='Output file name (default: icon_output.h)')
    parser.add_argument('-n', '--name', default='custom_icon',
                       help='Icon name for C array (default: custom_icon)')
    parser.add_argument('-b', '--batch', action='store_true',
                       help='Batch convert all PNGs in folder')
    parser.add_argument('-p', '--preview', action='store_true',
                       help='Show ASCII preview')
    
    args = parser.parse_args()
    
    if args.batch:
        # Batch convert folder
        batch_convert(args.input, args.output)
    
    else:
        # Single file conversion
        if not os.path.isfile(args.input):
            print(f"❌ File not found: {args.input}")
            sys.exit(1)
        
        print(f"🔄 Converting {args.input} to 8x8 icon...")
        
        # Process single image
        img = resize_to_8x8(args.input)
        color_matrix = image_to_color_matrix(img)
        color_matrix = apply_brightness_scaling(color_matrix)
        
        if args.preview:
            preview_ascii(color_matrix)
        
        # Generate array
        c_array = generate_c_array(color_matrix, args.name, args.output)
        
        print(f"\n✅ Conversion complete! Array saved to {args.output}")
        print("\n📋 Copy this into your icons.h file:\n")
        print(c_array)

if __name__ == "__main__":
    main()