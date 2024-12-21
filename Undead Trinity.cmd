@echo off
set compiler="..\Papyrus Compiler\PapyrusCompiler.exe"
set options=-op -o=Scripts -i=Scripts\Source -f=TESV_Papyrus_Flags.flg
%compiler% UT_Actor.psc %options%
if %errorlevel% neq 0 goto stop
%compiler% UT_Effect.psc %options%
if %errorlevel% neq 0 goto stop
%compiler% UT_Trinity.psc %options%
if %errorlevel% neq 0 goto stop
goto done
:stop
pause
:done
