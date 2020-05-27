source ./.venv/bin/activate.fish
set -x PYTHONPATH ../INSTALL
coverage run tests/test_main.py
coverage html
#sensible-browser ./coverage/index.html
