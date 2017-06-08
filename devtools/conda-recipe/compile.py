import fnmatch
import os
import subprocess


def build_all(input_dir, output_dir):
    java_files = []
    for d, subdirs, files in os.walk(input_dir):
        files = fnmatch.filter(files, '*.java')
        for f in files:
            java_files.append(os.path.join(d, f))

    cmd = ['javac', '-d', output_dir] + java_files
    subprocess.check_call(cmd)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_dir', default='test/harness')
    parser.add_argument('--output_dir', default='test/classes')
    args = parser.parse_args()
    os.makedirs(args.output_dir, exist_ok=True)
    build_all(input_dir=args.input_dir, output_dir=args.output_dir)
