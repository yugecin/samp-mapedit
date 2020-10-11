samp-mapedit
============

An in-game map editor for SA:MP/GTA San Andreas. The editor is backed by SA:MP,
to ensure that all SA:MP specific functions (such as SetObjectMaterial and
SetObjectMaterialText) will behave exactly as shown in the editor.

demo/images: https://twitter.com/yugecin26/status/1255233630051487744

Install
-------
* certain functions might not work when Silent Patch is used *

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
server-plugin: the plugin for the backing SA:MP server
shared: code shared between client and server

Acknowledgements
----------------
This wouldn't have been possible without following resources:
- Plugin-SDK: https://github.com/DK22Pac/plugin-sdk
- MTA:SA: https://github.com/multitheftauto/mtasa-blue
- The IDA database I have. I can't remember where I found it though.
- Sanny Builder and CLEO: https://sannybuilder.com

Note
----
Files in server-plugin/vendor/SDK/amx
are Copyright (c) ITB CompuPhase, 1997-2005
Files in server-plugin/vendor/SDK (excluding the amx/ directory)
are Copyright 2004-2009 SA-MP Team
Modifications in these files are marked as such with comments.
