// log.h — proste logowanie do native_banks.log obok DLL.
#pragma once
void LogInit();
void LogInfo(const char* fmt, ...);
void LogError(const char* fmt, ...);
void LogDebug(const char* fmt, ...);   // tylko gdy Config::debug
