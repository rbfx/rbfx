
venv:
	python3 -m venv ../cmake-build/python-venv
	../cmake-build/python-venv/bin/pip install -r AutoSwig/requirements.txt

autoswig:
	../cmake-build/python-venv/bin/python AutoSwig/autoswig.py ../cmake-build/Source/Urho3D/generator_options_Urho3D.txt
