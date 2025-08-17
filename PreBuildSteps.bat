@echo off
setlocal

set VS2022="C:\Program Files\Microsoft Visual Studio\2022\Community"
if not exist %VS2022% (
  echo VS2022 not found, please adjust path.
  exit /b 1
)

call %VS2022%\VC\Auxiliary\Build\vcvars64.bat

set RUSTUP_TOOLCHAIN=stable-x86_64-pc-windows-msvc

cd /d "%~dp0"  
cargo build --release --target x86_64-pc-windows-msvc
cargo build --target x86_64-pc-windows-msvc


