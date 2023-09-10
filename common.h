#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

// On Windows, the incoming stack is aligned to a 4 byte boundary.
// force_align_arg_pointer will realign the stack to match GCC's 16 byte alignment.
#define WIN_ENTRY __attribute__((force_align_arg_pointer, callee_pop_aggregate_return(0)))
#define WIN_FUNC WIN_ENTRY __attribute__((stdcall))
#define DEBUG_LOG(...) wibo::debug_log(__VA_ARGS__)

namespace user32 {
	int WIN_FUNC MessageBoxA(void *hwnd, const char *lpText, const char *lpCaption, unsigned int uType);
}

namespace wibo {
	struct WiboConfig {
		char *commandLine;
		bool debugEnabled;
		bool poitinEnabled;
	};

	extern uint32_t lastError;
	extern WiboConfig wiboConfig;

	void debug_log(const char *fmt, ...);

	void *resolveVersion(const char *name);
	void *resolveKernel32(const char *name);
	void *resolveUser32(const char *name);
	void *resolveOle32(const char *name);
	void *resolveAdvApi32(const char *name);
	void *resolveLmgr(uint16_t ordinal);
	void *resolveFuncByName(const char *dllName, const char *funcName);
	void *resolveFuncByOrdinal(const char *dllName, uint16_t ordinal);

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