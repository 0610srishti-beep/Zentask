@echo off
setlocal
cd /d "%~dp0"
echo Building ZenTask web app...
gcc -std=c11 -Wall -Wextra -Iinclude src\task.c src\file_io.c src\hash_table.c src\linked_list.c src\priority_heap.c src\web_server.c -o zentask-web.exe -lws2_32
if errorlevel 1 (
  echo.
  echo Build failed. Check the compiler output above.
  pause
  exit /b 1
)
echo.
echo Build successful. Starting ZenTask...
echo Open http://127.0.0.1:8080 in your browser.
echo Keep this window open while using ZenTask. Press Ctrl+C to stop.
echo.
zentask-web.exe
