import re

def sanitize_label(text):
    """
    Sanitize the label text by removing or replacing special characters.
    Only allow alphanumeric characters and underscores.
    """
    # Replace spaces with underscores
    text = text.replace(" ", "_")
    # Remove any characters that are not alphanumeric or underscores
    text = re.sub(r"[^\w]", "", text)
    return text.lower()


def sync_labels(input_file, output_file):
    """
    Synchronize labels with headers in an RST file. Labels are generated based on the chapter, header, subheader, and 
    sub-subheader structure. The script ensures anchors appear immediately before the correct header text, preserves 
    all section content, and avoids duplicating labels.

    Header Hierarchy:
    - Chapters are identified by lines underlined with `*`.
    - Headers are identified by lines underlined with `=`.
    - Subheaders are identified by lines underlined with `-`.
    - Sub-subheaders are identified by lines underlined with `~`.

    Label Format:
    - Labels follow the format: `.. _chapter_header_subheader_subsubheader:`
    - Labels are generated dynamically based on the text of the header and its position in the hierarchy.
    """
    # Regular expressions to identify underline patterns for different header levels
    underline_patterns = {
        "*": "chapter",
        "=": "header",
        "-": "subheader",
        "~": "subsubheader",
    }

    # Variables to track the current hierarchy of headers
    current_chapter = None
    current_header = None
    current_subheader = None

    # Read the input file
    with open(input_file, "r") as infile:
        lines = infile.readlines()

    # Initialize output lines
    output_lines = []
    buffer = None  # Holds the previous line to check for headers
    last_label = None
    line_count = 0

    for i, line in enumerate(lines):
        # Debugging: Print the current line being processed
        print(f"Processing line {i}: {line.strip()}")

        if line.startswith(".."):
            last_label = line
            line_count = 0

        # Check if the line is an underline pattern
        if re.match(r"^\*+$", line):
            # Process chapter
            current_chapter = sanitize_label(buffer.strip().lower().replace(" ", "_")) if buffer else None
            label = f".. _{current_chapter}:\n\n" if current_chapter else None
            if label and not line_count == 2:
                print(f"Detected chapter: {current_chapter}, adding label: {label.strip()}")
                output_lines.append(label)
            if buffer:
                print(f"Detected chapter (skip): {current_chapter}, adding label: {label.strip()}")
                output_lines.append(buffer)  # Add the header text immediately after the label
            output_lines.append(line)  # Add the underline itself
            buffer = None
        elif re.match(r"^=+$", line):
            # Process header
            current_header = sanitize_label(buffer.strip().lower().replace(" ", "_")) if buffer else None
            if not current_header:  # Fallback for empty buffer
                current_header = "unknown_header"
            label = f".. _{current_chapter}_{current_header}:\n\n" if current_header else None
            if label and not line_count == 2:
                print(f"Detected header: {current_header}, adding label: {label.strip()}")
                output_lines.append(label)
            if buffer:
                print(f"Detected header (skip): {current_chapter}, adding label: {label.strip()}")
                output_lines.append(buffer)  # Add the header text immediately after the label
            output_lines.append(line)  # Add the underline itself
            buffer = None
        elif re.match(r"^-+$", line):
            # Process subheader
            current_subheader = sanitize_label(buffer.strip().lower().replace(" ", "_")) if buffer else None
            label = f".. _{current_chapter}_{current_subheader}:\n\n" if current_subheader else None
            if label and not line_count == 2:
                print(f"Detected subheader: {current_subheader}, adding label: {label.strip()}")
                output_lines.append(label)
            if buffer:
                output_lines.append(buffer)  # Add the header text immediately after the label
            output_lines.append(line)  # Add the underline itself
            buffer = None
        elif re.match(r"^~+$", line):
            # Process sub-subheader
            subsubheader_text = sanitize_label(buffer.strip().lower().replace(" ", "_")) if buffer else None
            label = f".. _{current_chapter}_{subsubheader_text}:\n\n" if subsubheader_text else None
            if label and not line_count == 2:
                print(f"Detected sub-subheader: {subsubheader_text}, adding label: {label.strip()}")
                output_lines.append(label)
            if buffer:
                output_lines.append(buffer)  # Add the header text immediately after the label
            output_lines.append(line)  # Add the underline itself
            buffer = None
        else:
            # If the line isn't an underline, store it in the buffer
            if buffer:
                output_lines.append(buffer)  # Add the previous line to the output
            buffer = line  # Store the current line for processing
            if not line.isspace():
                line_count += 1

    if buffer is not None:
        output_lines.append(buffer)

    # Write the output to the specified file
    with open(output_file, "w") as outfile:
        outfile.writelines(output_lines)

    print(f"Labels synchronized successfully! Output written to {output_file}")


# Input and output file paths
input_file = "userguide.rst"
output_file = "userguide_with_synced_labels.rst"

# Run the label synchronization
sync_labels(input_file, output_file)
