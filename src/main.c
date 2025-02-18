#include "gb.h"
#include "gui.h"
#include <argp.h>

const char *argp_program_version = "gamegirl 0.0.1";
//const char *argp_program_bug_address = "<bug-gnu-utils@gnu.org>";

// Program documentation.
static char argp_doc[] = "GameGirl, a GameBoy(tm) Emulator by Haley";
// A description of the positional arguments we accept
static char argp_positional_doc[] = "romImage";
// The options we understand.
static struct argp_option argp_options[] = {
  {"foo",       'f', 0,      0,  "Boolean Test option" },
  {"break",     'b', "ADDR", 0,  "Set Breakpoint at address [ADDR]"},
  {"debugLog",  'd', "FILE", 0,  "Output Gameboy-Doctor compatible log to [FILE]" },
  { 0 }
};

static error_t argpParser(int key, char *arg, struct argp_state *state);

// Our argp parser.
static struct argp argp_config = { argp_options, argpParser, argp_positional_doc, argp_doc };

/* Used by main to communicate with parse_opt. */
struct ArgResult
{
  char *romFilename;
  bool foo;
  bool breakpointSet;
  int breakpoint;
  char *debugLog;
};

// argp callback to process a single option
static error_t argpParser(int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct ArgResult *args = (struct ArgResult *)state->input;

  switch (key)
    {
    case 'f':
      args->foo = true;
      break;
    case 'b':
      args->breakpointSet = true;
      sscanf(arg,"%x", &args->breakpoint);
      break;
    case 'd':
      args->debugLog = arg;
      break;
    case ARGP_KEY_ARG:
      args->romFilename = arg;
      break;

    case ARGP_KEY_END:
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

FILE *doctorLogFile = NULL;

int main(int argc, char **argv)
{
    struct ArgResult args;
    memset(&args, 0, sizeof(args));

    // parse args
    argp_parse(&argp_config, argc, argv, 0, 0, &args);

    if(0 != args.debugLog) {
        printf("Enabling Gameboy-Doctor log output to '%s'\n", args.debugLog);
        if( NULL == (doctorLogFile = fopen(args.debugLog, "w")) ) {
            printf("Error opening debug log file!\n");
            exit(0);
        }

    }

    systemBreakpoint = (args.breakpointSet)? args.breakpoint : 0xFFFF;

    gbInit(args.romFilename);

    /*
    if( argc > 1 ) {
        //dumpMemory(cartridge.rom.contents, cartridge.rom.size);
        //disassembleRom(&cartridge.rom);
        //unloadCartridge(&cartridge);
    } else {
        //dumpMemory(bootrom.contents, bootrom.size);
        //disassembleRom(&bootrom);
        }*/


    gui();

    gbDeinit();
    if(NULL != doctorLogFile) {
        fclose(doctorLogFile);
    }
    return 0;
}
