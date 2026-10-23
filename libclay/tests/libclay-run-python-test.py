#!/usr/bin/env python3
import os
import sys
import subprocess
from pathlib import Path

CLAY_EXE = sys.argv[1]
TEST_FILE = sys.argv[2]
SRC_DIR = os.path.dirname(TEST_FILE)
SRC_DIR = Path(os.path.realpath(SRC_DIR)).resolve().as_posix()
PYCLAY_DIR = os.path.dirname(__file__)
PYCLAY_DIR = Path(os.path.realpath(PYCLAY_DIR)).resolve().as_posix()

if not os.path.isfile(TEST_FILE):
  print(f"ERROR: file '{TEST_FILE}' does not exist!")
  sys.exit(1)

with open(TEST_FILE, 'r') as f:
  first_char = f.read(1)

if first_char != '#':
  # Note: I don't actually care that it's a shebang, just that it's a comment,
  # hence a useless line, because I'm going to remove it and replace it with
  # clay_assert() import line.
  # This will make much easier to debug tests with meaningful line numbers.
  print(f"ERROR: file '{TEST_FILE}' should start with a shebang: #!/usr/bin/env python3")
  sys.exit(1)

header = f"""import os; import sys; sys.path.insert(0, '{PYCLAY_DIR}'); from pyclay.utils import clay_assert;
import pyclay.utils; pyclay.utils.clay_test_filename = '{TEST_FILE}'"""

with open(TEST_FILE, 'r') as f:
  content = f.readlines()
cmd = header + ''.join(content[1:])
subprocess.run([sys.executable, CLAY_EXE, "-nis", "--batch-interpreter", "python-fu-eval", "-b", cmd, "--quit"], check=True)
