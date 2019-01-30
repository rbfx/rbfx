
venv:
	python3 -m venv ../build/python-venv
	../build/python-venv/bin/pip install clang

autoswig:
	../build/python-venv/bin/python AutoSwig/autoswig.py ../build/Source/Urho3D/CSharp/generator_options.txt
