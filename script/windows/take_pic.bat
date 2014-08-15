@set input=1231
@set /p input="%1 %2 ，回车开始,a+回车取消:"
@if "%input%"=="a" goto next1
@take_pic_repeat.bat %2
:next1

