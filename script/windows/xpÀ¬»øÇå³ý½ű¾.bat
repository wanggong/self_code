@echo off
color 3b
title   清除XP系统垃圾工具
echo 正在清除系统垃圾文件，请稍等......
rem 提示部份软件卸载需LOG 文件，清理掉后就无法卸载
if exist "%windir%\*.log" del /f /s /q "%windir%\*.log" 2>nul
if exist "%systemdrive%\*.log" del /f /s /q "%systemdrive%\*.log" 2>nul
if exist "%systemdrive%\*.tmp" del /f /s /q "%systemdrive%\*.tmp" 2>nul
if exist "%systemdrive%\*._mp" del /f /s /q "%systemdrive%\*._mp" 2>nul
if exist "%systemdrive%\*.gid" del /f /s /q "%systemdrive%\*.gid" 2>nul
if exist "%systemdrive%\*.chk" del /f /s /q "%systemdrive%\*.chk" 2>nul
if exist "%systemdrive%\*.old" del /f /s /q "%systemdrive%\*.old" 2>nul
if exist "%systemdrive%\*.ftg" del /f /s /q "%systemdrive%\*.ftg" 2>nul
if exist "%systemdrive%\*.fts" del /f /s /q "%systemdrive%\*.fts" 2>nul
if exist "%systemdrive%\*._mp" del /f /s /q "%systemdrive%\*._mp" 2>nul
if exist "%systemdrive%\*.syd" del /f /s /q "%systemdrive%\*.syd" 2>nul
if exist "%systemdrive%\*.ms" del /f /s /q "%systemdrive%\*.ms" 2>nul
if exist "%systemdrive%\*.diz" del /f /s /q "%systemdrive%\*.diz" 2>nul
if exist "%systemdrive%\*.??$" del /f /s /q "%systemdrive%\*.??$" 2>nul
if exist "%systemdrive%\*.??~" del /f /s /q "%systemdrive%\*.??~" 2>nul
if exist "%systemdrive%\*.$$$" del /f /s /q "%systemdrive%\*.$$$" 2>nul
if exist "%systemdrive%\*.~*" del /f /s /q "%systemdrive%\*.~*" 2>nul
if exist "%systemdrive%\*.@@@" del /f /s /q "%systemdrive%\*.@@@" 2>nul
if exist "%systemdrive%\recycled\*.*" del /f /s /q %systemdrive%\recycled\*.* 2>nul
if exist "%windir%\*.bak" del /f /s /q %windir%\*.bak 2>nul
if exist "%windir%\*.tmp" del /f /s /q %windir%\*.tmp 2>nul
if exist "%windir%\SoftwareDistribution\Download\*.*" del /f /s /q %windir%\SoftwareDistribution\Download\*.* 2>nul
if exist "%systemroot%\temp\*.*" del /a /f /s /q "%systemroot%\temp\*.*" 2>nul
if exist "%systemroot%\Prefetch\*.*" del /a /f /s /q "%systemroot%\Prefetch\*.*" 2>nul
if exist "%systemroot%\minidump\*.*" del /f /q /s /a "%systemroot%\minidump\*.*" 2>nul
if exist "%systemroot%\*.dmp" del /a /f /s /q "%systemroot%\*.dmp" 2>nul
if exist "%systemroot%\*.tmp" del /a /f /s /q "%systemroot%\*.tmp" 2>nul
if exist "%systemroot%\*._mp" del /a /f /s /q "%systemroot%\*._mp" 2>nul
if exist "%systemroot%\*.gid" del /a /f /s /q "%systemroot%\*.gid" 2>nul
if exist "%systemroot%\*.bak" del /a /f /s /q "%systemroot%\*.bak" 2>nul
if exist "%systemroot%\*.old" del /a /f /s /q "%systemroot%\*.old" 2>nul
if exist "%systemroot%\*.query" del /a /f /s /q "%systemroot%\*.query" 2>nul
rd /s /q %windir%\temp & md %windir%\temp
if exist "%userprofile%\Local Settings\Temp\*.*" del /f /s /q "%userprofile%\Local Settings\Temp\*.*" 2>nul
if exist "%userprofile%\recent\*.*" del /f /s /q "%userprofile%\recent\*.*" 2>nul
if exist "%ALLUSERSPROFILE%\Documents\DrWatson\*.*" del /f /q "%ALLUSERSPROFILE%\Documents\DrWatson\*.*" 2>nul
if exist "%USERPROFILE%\Application Data\Microsoft\Office\Recent\*.lnk" del /f /q "%USERPROFILE%\Application Data\Microsoft\Office\Recent\*.lnk" 2>nul
rem echo 正在清理临时文件……
for /d %%a in ("%temp%\*.*") do rd /s /q "%%a"
del /a /f /s /q "%temp%\*.*" 2>nul
for /d %%a in ("%tmp%\*.*") do rd /s /q "%%a"
del /a /f /s /q "%tmp%\*.*" 2>nul
echo 正在清理系统升级补丁留下来的反安装目录……
for /f "tokens=1" %%i in ('dir %SystemRoot%\$*$ /s /b /ad') do rd /s /q %%i
echo 正在正在清理IE缓存、cookies、历史纪录等(当前用户)...
reg query "HKCU\software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v Cache>vtemp.txt
reg query "HKCU\software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v Cookies>>vtemp.txt
reg query "HKCU\software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v History>>vtemp.txt
reg query "HKCU\software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v NetHood>>vtemp.txt
reg query "HKCU\software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v Recent>>vtemp.txt
for /f "tokens=3*" %%a in (vtemp.txt) do (
for /d %%i in ("%%a %%b\*.*") do rd /s /q "%%i" 2>Nul
del /a /f /s /q "%%a %%b\*.*" del /q vtemp.txt 2>Nul
)
echo 清除浏览器地址记录...
reg delete "HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\TypedUrls" /f 2>nul
REG ADD "HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\TypedUrls" 
echo 清除运行记录...
reg delete "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\RunMRU" /f 2>nul
REG ADD "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\RunMRU" /v "MRUList" /t reg_sz /d "a" /f           
pause >nul
goto exit