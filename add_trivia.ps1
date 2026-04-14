<#
add_trivia.ps1 — run scripts\add_trivia.py using the repository .venv Python when available.
Usage: .\add_trivia.ps1 [path-to-trivia.json]
#>
param([Parameter(ValueFromRemainingArguments=$true)] $Args)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$venv = Join-Path $scriptDir ".venv\Scripts\python.exe"
$script = Join-Path $scriptDir "scripts\add_trivia.py"

if (Test-Path $venv) {
    & $venv $script @Args
} else {
    & python $script @Args
}
