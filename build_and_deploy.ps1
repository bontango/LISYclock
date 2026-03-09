#Requires -Version 5.1
# build_and_deploy.ps1
#
# Baut LISYclock fuer beide Hardware-Versionen (v1.x und v2.x),
# kopiert die Binaries nach releases\ und laedt sie per SFTP (WinSCP) hoch.
#
# Voraussetzungen:
#   - WinSCP installiert (wird automatisch gefunden)
#   - .env Datei im Projektverzeichnis (siehe .env.example)
#
# Verwendung:
#   .\build_and_deploy.ps1

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

# -- Hilfsfunktionen ----------------------------------------------------------
function Info([string]$msg)    { Write-Host "[INFO]  $msg" -ForegroundColor Green }
function Warn([string]$msg)    { Write-Host "[WARN]  $msg" -ForegroundColor Yellow }
function Err([string]$msg)     { Write-Host "[ERROR] $msg" -ForegroundColor Red }
function StepMsg([string]$msg) { Write-Host "`n== $msg ==" -ForegroundColor Cyan }

function Write-FileUtf8NoBom([string]$path, [string]$content) {
    $enc = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText((Resolve-Path $path).Path, $content, $enc)
}

# -- WinSCP suchen ------------------------------------------------------------
$winscpPaths = @(
    "C:\Program Files (x86)\WinSCP\WinSCP.com",
    "C:\Program Files\WinSCP\WinSCP.com"
)
$winscpExe = $null
foreach ($p in $winscpPaths) {
    if (Test-Path $p) { $winscpExe = $p; break }
}
if (-not $winscpExe) {
    $winscpExe = Get-Command WinSCP.com -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}
if (-not $winscpExe) {
    Err "WinSCP.com nicht gefunden. Bitte WinSCP installieren: https://winscp.net"
    exit 1
}
Info "WinSCP gefunden: $winscpExe"

# -- .env laden ---------------------------------------------------------------
if (-not (Test-Path ".env")) {
    Err ".env nicht gefunden."
    Err "Bitte .env.example nach .env kopieren und mit echten SFTP-Daten befuellen."
    exit 1
}
$envVars = @{}
Get-Content ".env" | Where-Object { $_ -match '^\s*[^#\s].+=.' } | ForEach-Object {
    $parts = $_ -split "=", 2
    $envVars[$parts[0].Trim()] = $parts[1].Trim()
}

foreach ($var in @("SFTP_HOST", "SFTP_USER", "SFTP_PATH")) {
    if (-not $envVars.ContainsKey($var) -or [string]::IsNullOrEmpty($envVars[$var])) {
        Err "Variable '$var' fehlt oder ist leer in .env"
        exit 1
    }
}

$SFTP_HOST = $envVars["SFTP_HOST"]
$SFTP_USER = $envVars["SFTP_USER"]
$SFTP_PATH = $envVars["SFTP_PATH"]
if ($envVars["LOCAL_RELEASES_DIR"]) {
    $LOCAL_RELEASES_DIR = $envVars["LOCAL_RELEASES_DIR"]
} else {
    $LOCAL_RELEASES_DIR = "releases"
}

# -- SFTP-Passwort abfragen ---------------------------------------------------
$securePass = Read-Host "SFTP-Passwort fuer ${SFTP_USER}@${SFTP_HOST} (Enter = nur lokal kopieren)" -AsSecureString
$bstr       = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($securePass)
$SFTP_PASS  = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto($bstr)
[System.Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr) | Out-Null

if ([string]::IsNullOrEmpty($SFTP_PASS)) {
    Warn "Kein Passwort eingegeben - SFTP-Upload wird uebersprungen."
    $SFTP_ENABLED = $false
} else {
    $SFTP_ENABLED = $true
}

# -- ESP-IDF Umgebung ---------------------------------------------------------
if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    $IdfExport = "$env:USERPROFILE\esp\v5.5.1\esp-idf\export.ps1"
    if (Test-Path $IdfExport) {
        if (-not $env:IDF_PYTHON_ENV_PATH) {
            $env:IDF_PYTHON_ENV_PATH = "$env:USERPROFILE\.espressif\python_env\idf5.5_py3.11_env"
        }
        Info "Lade ESP-IDF Umgebung (venv: $env:IDF_PYTHON_ENV_PATH) ..."
        . $IdfExport
    } else {
        Err "idf.py nicht im PATH und $IdfExport nicht gefunden."
        Err "Bitte ESP-IDF Terminal verwenden oder Pfad anpassen."
        exit 1
    }
}

# -- Versionen aus gpiodefs.h auslesen ----------------------------------------
$GPIODEFS = "main\gpiodefs.h"
$content  = [System.IO.File]::ReadAllText((Resolve-Path $GPIODEFS).Path)

if ($content -match '(?s)#ifdef LISYCLOCK2.*?LISYCLOCK_VERSION\s+"(v[^" ]+)') {
    $V2 = $Matches[1]
} else {
    Err "v2 Versionsstring nicht gefunden in $GPIODEFS"
    exit 1
}

if ($content -match '(?s)#else.*?LISYCLOCK_VERSION\s+"(v[^" ]+)') {
    $V1 = $Matches[1]
} else {
    Err "v1 Versionsstring nicht gefunden in $GPIODEFS"
    exit 1
}

Info "Versionen erkannt: HW 1.x -> $V1 | HW 2.x -> $V2"

# -- Verzeichnis vorbereiten --------------------------------------------------
New-Item -ItemType Directory -Force -Path $LOCAL_RELEASES_DIR | Out-Null

# -- Restore-Funktion bei Fehler ----------------------------------------------
function Restore-V2 {
    Warn "Stelle gpiodefs.h auf HW v2.x zurueck..."
    $c = [System.IO.File]::ReadAllText((Resolve-Path $GPIODEFS).Path)
    $c = $c -replace '(?m)^//\s*#define LISYCLOCK2 TRUE', '#define LISYCLOCK2 TRUE'
    Write-FileUtf8NoBom $GPIODEFS $c
}

# -- Hilfsfunktion: Build + Copy + SFTP (WinSCP) ------------------------------
function Build-And-Deploy([string]$version) {
    $outfile  = "$LOCAL_RELEASES_DIR\LISYclock_${version}.bin"
    $sftpName = "LISYclock_${version}.bin"

    Info "Starte idf.py build ..."
    idf.py build
    if ($LASTEXITCODE -ne 0) { throw "idf.py build fehlgeschlagen (exit $LASTEXITCODE)" }

    Info "Kopiere build\LISYclock.bin -> $outfile"
    Copy-Item "build\LISYclock.bin" $outfile -Force

    if ($SFTP_ENABLED) {
        Info "SFTP-Upload: $sftpName -> sftp://${SFTP_HOST}${SFTP_PATH}/"

        # WinSCP-Script in temporaere Datei schreiben (Passwort bleibt aus Prozessliste)
        $tmpScript = [System.IO.Path]::GetTempFileName()
        @"
open sftp://${SFTP_USER}@${SFTP_HOST}/ -password="$SFTP_PASS" -hostkey=*
put "$outfile" "${SFTP_PATH}/${sftpName}"
exit
"@ | Set-Content $tmpScript -Encoding UTF8

        try {
            & $winscpExe /ini=nul /script=$tmpScript
            if ($LASTEXITCODE -ne 0) { throw "WinSCP exit $LASTEXITCODE" }
        } finally {
            Remove-Item $tmpScript -ErrorAction SilentlyContinue
        }

        Info "OK: $sftpName erfolgreich hochgeladen."
    }
}

# -- Builds durchfuehren ------------------------------------------------------
try {
    StepMsg "Build HW v1.x ($V1)"
    Info "Kommentiere LISYCLOCK2 aus (-> HW v1.x) ..."
    $c = [System.IO.File]::ReadAllText((Resolve-Path $GPIODEFS).Path)
    $c = $c -replace '(?m)^#define LISYCLOCK2 TRUE', '// #define LISYCLOCK2 TRUE'
    Write-FileUtf8NoBom $GPIODEFS $c
    Build-And-Deploy $V1

    StepMsg "Build HW v2.x ($V2)"
    Info "Aktiviere LISYCLOCK2 (-> HW v2.x) ..."
    $c = [System.IO.File]::ReadAllText((Resolve-Path $GPIODEFS).Path)
    $c = $c -replace '(?m)^//\s*#define LISYCLOCK2 TRUE', '#define LISYCLOCK2 TRUE'
    Write-FileUtf8NoBom $GPIODEFS $c
    Build-And-Deploy $V2
}
catch {
    Err "Build abgebrochen: $_"
    Restore-V2
    exit 1
}

# -- Fertig -------------------------------------------------------------------
Write-Host ""
Write-Host "==================================================" -ForegroundColor Green
Info "Build & Deploy erfolgreich abgeschlossen!"
Info "Lokal gespeichert:"
Info "  $LOCAL_RELEASES_DIR\LISYclock_${V1}.bin"
Info "  $LOCAL_RELEASES_DIR\LISYclock_${V2}.bin"
if ($SFTP_ENABLED) {
    Info "SFTP-Ziel: sftp://${SFTP_HOST}${SFTP_PATH}/"
} else {
    Warn "SFTP-Upload wurde uebersprungen (kein Passwort eingegeben)."
}
Write-Host "==================================================" -ForegroundColor Green
