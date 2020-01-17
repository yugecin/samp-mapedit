samp-mapedit
------------

An in-game map editor for SA:MP/GTA San Andreas. The editor is backed by SA:MP,
to ensure that all SA:MP specific functions (such as SetObjectMaterial and
SetObjectMaterialText) will behave exactly as shown in the editor. The editor
can also run without SA:MP, but then all specific SA:MP functionality won't
work.

Anatomy
-------
client: the client
client-loader-cleo: the cleo plugin and script that loads the client
client-mpack: the missionpack to load the editor without the backing server
server-plugin: the plugin for the backing SA:MP server
shared: code shared between client and server

Note
----
Files in server-plugin/vendor/SDK/amx
are Copyright (c) ITB CompuPhase, 1997-2005
Files in server-plugin/vendor/SDK (excluding the amx/ directory)
are Copyright 2004-2009 SA-MP Team
Modifications in these files are marked as such with comments.
