@echo off
:a

COPY /Y out-debug\server-plugin.dll server\plugins\mapedit-server.dll
COPY /Y out-debug\server-plugin.pdb server\plugins\mapedit-server.pdb

pushd server
"samp-server.exe"
popd
echo :: server exited, restarting in ~3
sleep 3
goto a
