samp-mapedit
------------

An in-game map editor for SA:MP/GTA San Andreas. The editor is backed by SA:MP,
to ensure that all SA:MP specific functions (such as SetObjectMaterial and
SetObjectMaterialText) will behave exactly as shown in the editor. The editor
can also run without SA:MP, but then all specific SA:MP functionality won't
work.

Install
-------
- install CLEO (https://cleo.li)
- copy mapedit-client-loader-cleo.cleo to CLEO folder
- copy mapedit.cs to CLEO folder
- (optional: make mapedit-client.dll.txt in CLEO folder with as contents the
  path to where mapedit-client.dll gets compiled, for the reload functionality
  to work)
- (if not doing the above, copy mapedit-client.dll to CLEO folder)
- get a sa-mp server, add mapedit-server.dll to plugins and use the cfg from
  the server dir

Run
---
- run the sa-mp server
- join it
- hold F9 for ~5 seconds
- a small text should show at the bottom of the screen

Manual
------
todo

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

TODO
----
test saving project while in vehicle (to test game_PedGetPos)
test loading project while in vehicle
