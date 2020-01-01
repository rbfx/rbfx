Editor notes
============

This document maintains a list of details developer should be aware of when working on the editor.

### Serialization of editor objects

We have extra objects when editing scene in the editor. Such objects can be helper icons indicating type of 3d object 
(lights for example), developer camera and others. Such objects are tagged with `__EDITOR_OBJECT__` tag. This tag is 
used to save/restore state of editor objects and then restore them when scene simulation is stopped. This hack should be
reworked into a more proper solution eventually.

### Last rendering layer

Editor uses last layer (`1U << 31U`) for rendering helper objects (debug icons for lights, cameras, etc). This layer is 
enabled for editor viewport camera, but disabled for all other user cameras in the scene. Enabling this layer would also
render helper objects in game preview, which we do not want. If this layer is enabled various interactions may disable 
it again automatically.
