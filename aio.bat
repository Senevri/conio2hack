@echo off
cd /d "E:\docker\llm-agent\workspace\conio2hack"
"C:\MinGW\bin\gcc.exe" -std=c99 "src\main.c" "src\edit.c" "src\hack.c" "ext\conio21\conio.c" -I"ext\conio21" -I"src" -o "bin\test.exe" -Wall
echo gcc exit code: %errorlevel%
