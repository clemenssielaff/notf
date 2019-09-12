source ./.venv/bin/activate.fish
coverage run tests/test_main.py
coverage html
#sensible-browser ./coverage/index.html
