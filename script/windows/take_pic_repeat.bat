:retake
call auto_take_pic.bat %1
set input=1231
set /p input="重拍:R,下一个:ENTER:"
if "%input%"=="r" goto retake
