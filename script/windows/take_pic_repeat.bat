:retake
call auto_take_pic.bat %1
set input=1231
set /p input="����:R,��һ��:ENTER:"
if "%input%"=="r" goto retake
