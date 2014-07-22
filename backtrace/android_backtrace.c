#include <corkscrew/backtrace.h>
#include <corkscrew/symbol_table.h>
	
static int do_backtrace() {
  const size_t MAX_DEPTH = 32; 
  size_t i = 0;
  backtrace_frame_t* frames = (backtrace_frame_t*) malloc(sizeof(backtrace_frame_t) * MAX_DEPTH);
  ssize_t frame_count = unwind_backtrace(frames, 0, MAX_DEPTH);
  ALOGE( "frame_count=%d\n", (int) frame_count);
  if (frame_count <= 0) {
	return frame_count;
  }

  backtrace_symbol_t* backtrace_symbols = (backtrace_symbol_t*) malloc(sizeof(backtrace_symbol_t) * frame_count);
  get_backtrace_symbols(frames, frame_count, backtrace_symbols);

  for (i = 0; i < (size_t) frame_count; ++i) {
	char line[MAX_BACKTRACE_LINE_LENGTH];
	format_backtrace_line(i, &frames[i], &backtrace_symbols[i],
						  line, MAX_BACKTRACE_LINE_LENGTH);
	if (backtrace_symbols[i].symbol_name != NULL) {
	  // get_backtrace_symbols found the symbol's name with dladdr(3).
	  ALOGE("  %s\n", line);
	} else {
	  // We don't have a symbol. Maybe this is a static symbol, and
	  // we can look it up?
	  symbol_table_t* symbols = NULL;
	  if (backtrace_symbols[i].map_name != NULL) {
		symbols = load_symbol_table(backtrace_symbols[i].map_name);
	  }
	  const symbol_t* symbol = NULL;
	  if (symbols != NULL) {
		symbol = find_symbol(symbols, frames[i].absolute_pc);
	  }
	  if (symbol != NULL) {
		int offset = frames[i].absolute_pc - symbol->start;
		ALOGE("  %s (%s%+d)\n", line, symbol->name, offset);
	  } else {
		ALOGE( "  %s (\?\?\?)\n", line);
	  }
	  free_symbol_table(symbols);
	}
  }

  free_backtrace_symbols(backtrace_symbols, frame_count);
  free(backtrace_symbols);
  free(frames);
  return frame_count;
}

//注意:还需要在Android.mk中田间LOCAL_SHARED_LIBRARIES := libcorkscrew