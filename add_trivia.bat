REM @echo off
REM add_trivia runner — prefer repository .venv Python if present
setlocal
set "ROOT=%~dp0"
set "VENV_PY=%ROOT%.venv\Scripts\python.exe"
set "PY_SCRIPT=%ROOT%scripts\add_trivia.py"

if exist "%VENV_PY%" (
  "%VENV_PY%" "%PY_SCRIPT%" %*
) else (
  python "%PY_SCRIPT%" %*
)
pause
endlocal
