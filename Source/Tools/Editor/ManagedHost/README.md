Managed Editor Host
===================

This project is used to bootstrap managed runtime independently of it's runtime environment. Managed executable loads
editor library and hands off execution to native code. From then on editor will have access to a fully operational .net
runtime in it's process space without having to manually host .net runtime.
