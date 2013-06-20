@echo off
cls

echo ========================
echo    Langpack refresher
echo      Author: Robyer
echo ========================
echo.

set err=0
if "%~1" == "" (
	echo ERROR: You must specify language parameter!
	set err=1
) else (
	set "language=%~1"
	
	if not exist "..\..\langpacks\%language%" (
		echo ERROR: This language doesn't exists!
		set err=1
		goto usage
	)
)

if not "%~3" == "" (
	echo ERROR: You must specify 1 or 2 parameters only.
	echo NOTE: If you have path with spaces inside, suround it with "".
	set err=1
	goto usage
)

if not "%~2" == "" (
	set "oldLangpackPath=%~2"
	
	if not exist "..\..\langpacks\%language%\%oldLangpackPath%" (
		echo ERROR: Old langpack file doesn't exists!
		set err=1
		goto usage
	)
)

:usage
if "%err%"=="1" (
	echo.
	echo ========================
	echo Usage: refresh.bat language ["old langpack path"]
	echo Note:  path to old langpack must be relative to "langpacks\<language>\" folder
	echo.
	echo Example 1: refresh.bat czech
	echo Example 2: refresh.bat czech "Deprecated\old langpack\langpack_czech2.txt"
	echo.
	echo If you want own script in langpack dir, look at "langpacks\czech\refresh.bat"
	echo ========================
	echo.
	
	pause
	goto end
)

REM =========================

if not "%oldLangpackPath%" == "" (
	set phase=0
) else (
	set phase=1
)

echo Refreshing started...

:start

cd "..\..\tools\lpgen\"

mkdir "Plugins"
mkdir "Untranslated"

if "%phase%" == "0" (
	echo Loading strings from old langpack...
	cscript /nologo translate.js /out:"Plugins" /untranslated:"Untranslated" /outfile:"langpack_%language%.txt" /path:"..\..\langpacks\%language%\Plugins" /core:"..\..\langpacks\%language%\=CORE=.txt" /langpack:"..\..\langpacks\%language%\%oldLangpackPath%"
) else (
	REM load strings from recently created langpack (also to distribute strings between files)
	cscript /nologo translate.js /out:"Plugins" /untranslated:"Untranslated" /outfile:"langpack_%language%.txt" /path:"..\..\langpacks\%language%\Plugins" /core:"..\..\langpacks\%language%\=CORE=.txt" /langpack:"..\..\langpacks\%language%\Langpack_%language%.txt"
)
	
rm -r "../../langpacks/%language%/Plugins"
rm -r "../../langpacks/%language%/Untranslated"
rm -r "../../langpacks/%language%/=CORE=.txt"
rm -r "../../langpacks/%language%/Langpack_%language%.txt"

mkdir "..\..\langpacks\%language%\Plugins"
mkdir "..\..\langpacks\%language%\Untranslated"
copy "Plugins\" "..\..\langpacks\%language%\Plugins\"
copy "Untranslated\" "..\..\langpacks\%language%\Untranslated\"
copy "=CORE=.txt" "..\..\langpacks\%language%\=CORE=.txt"

rm -r "Plugins"
rm -r "Untranslated"
rm "=CORE=.txt"
rm "Langpack_%language%.txt"

cd "..\..\langpacks\tool\"
LangpackSuite.exe \q \p%language%

if not "%phase%" == "1" (
	echo Loading strings from new langpack...
	set "phase=1"
	goto start
)

:end
echo Refreshing finished!

REM =========================