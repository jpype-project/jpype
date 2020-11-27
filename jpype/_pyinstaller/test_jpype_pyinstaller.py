import os
from pathlib import Path
from subprocess import run

import jpype
import PyInstaller.__main__


fspath = getattr(os, 'fspath', str)


example_path = Path(__file__).parent.joinpath('example.py')


def test_start_and_stop(tmp_path):
    name = 'e'
    dist = tmp_path.joinpath('dist')
    work = tmp_path.joinpath('build')
    result = dist.joinpath(name, name)

    PyInstaller.__main__.run([
        '--name',
        name,
        '--distpath',
        fspath(dist),
        '--workpath',
        fspath(work),
        fspath(example_path),
    ])

    run([fspath(result)], check=True, cwd=fspath(tmp_path))
