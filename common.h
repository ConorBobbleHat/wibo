#include <netinet/in.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#ifndef COMMON_H
#define COMMON_H

// On Windows, the incoming stack is aligned to a 4 byte boundary.
// force_align_arg_pointer will realign the stack to match GCC's 16 byte alignment.
#define WIN_ENTRY __attribute__((force_align_arg_pointer, callee_pop_aggregate_return(0)))
#define WIN_FUNC WIN_ENTRY __attribute__((stdcall))
#define DEBUG_LOG(...) wibo::debug_log(__VA_ARGS__)

typedef void *HANDLE;
typedef void *HMODULE;
typedef void *PVOID;
typedef void *LPVOID;
typedef void *FARPROC;
typedef uint32_t DWORD;
typedef DWORD *PDWORD;
typedef DWORD *LPDWORD;
typedef int32_t LONG;
typedef LONG *PLONG;
typedef uint32_t ULONG;
typedef ULONG *PULONG;
typedef int64_t LARGE_INTEGER;
typedef LARGE_INTEGER *PLARGE_INTEGER;
typedef uintptr_t ULONG_PTR;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef uint16_t *LPWSTR;
typedef const uint16_t *LPCWSTR;
typedef int BOOL;
typedef BOOL *PBOOL;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef size_t SIZE_T;
typedef SIZE_T *PSIZE_T;
typedef unsigned char BYTE;

#define TRUE 1
#define FALSE 0

#define ERROR_SUCCESS 0
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_PATH_NOT_FOUND 3
#define ERROR_ACCESS_DENIED 5
#define ERROR_INVALID_HANDLE 6
#define ERROR_READ_FAULT 30
#define ERROR_HANDLE_EOF 38
#define ERROR_NOT_SUPPORTED 50
#define ERROR_INVALID_PARAMETER 87
#define ERROR_NEGATIVE_SEEK 131
#define ERROR_ALREADY_EXISTS 183

#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#define INVALID_HANDLE_VALUE ((HANDLE)-1)

typedef int NTSTATUS;
#define STATUS_SUCCESS ((NTSTATUS)0x00000000)
#define STATUS_INVALID_HANDLE ((NTSTATUS)0xC0000008)
#define STATUS_END_OF_FILE ((NTSTATUS)0xC0000011)
#define STATUS_NOT_SUPPORTED ((NTSTATUS)0xC00000BB)
#define STATUS_UNEXPECTED_IO_ERROR ((NTSTATUS)0xC00000E9)

typedef int HRESULT;
#define S_OK ((HRESULT)0x00000000)

namespace wibo {
	struct WiboConfig {
		char *commandLine;
		bool debugEnabled;
		bool poitinEnabled;
	};

	extern uint32_t lastError;
	extern char **argv;
	extern int argc;
	extern WiboConfig wiboConfig;

	void debug_log(const char *fmt, ...);

	using ResolveByName = void *(*)(const char *);
	using ResolveByOrdinal = void *(*)(uint16_t);
	struct Module {
		const char** names;
		ResolveByName byName;
		ResolveByOrdinal byOrdinal;
	};
	extern const Module *modules[];

	HMODULE loadModule(const char *name);
	void freeModule(HMODULE module);
	void *resolveFuncByName(HMODULE module, const char *funcName);
	void *resolveFuncByOrdinal(HMODULE module, uint16_t ordinal);

	struct Executable {
		Executable();
		~Executable();
		bool loadPE(FILE *file);

		void *imageBuffer;
		size_t imageSize;
		void *entryPoint;
		void *rsrcBase;

		template <typename T>
		T *fromRVA(uint32_t rva) {
			return (T *) (rva + (uint8_t *) imageBuffer);
		}

		template <typename T>
		T *fromRVA(T *rva) {
			return fromRVA<T>((uint32_t) rva);
		}
	};

	extern Executable *mainModule;
}

struct UNICODE_STRING {
	unsigned short Length;
	unsigned short MaximumLength;
	uint16_t *Buffer;
};

// Run Time Library (RTL)
struct RTL_USER_PROCESS_PARAMETERS {
	char Reserved1[16];
	void *Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
};

// Windows Process Environment Block (PEB)
struct PEB {
	char Reserved1[2];
	char BeingDebugged;
	char Reserved2[1];
	void *Reserved3[2];
	void *Ldr;
	RTL_USER_PROCESS_PARAMETERS *ProcessParameters;
	char Reserved4[104];
	void *Reserved5[52];
	void *PostProcessInitRoutine;
	char Reserved6[128];
	void *Reserved7[1];
	unsigned int SessionId;
};

// Windows Thread Information Block (TIB)
struct TIB {
	/* 0x00 */ void *sehFrame;
	/* 0x04 */ void *stackBase;
	/* 0x08 */ void *stackLimit;
	/* 0x0C */ void *subSystemTib;
	/* 0x10 */ void *fiberData;
	/* 0x14 */ void *arbitraryDataSlot;
	/* 0x18 */ TIB *tib;
	/*      */ char pad[0x14];
	/* 0x30 */ PEB *peb;
	/*      */ char pad2[0x1000];
};

#endif