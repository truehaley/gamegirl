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
  {"debuglog", 'd', 0,      0,  "Produce gameboy-doctor compatible logs" },
  //{"output",   'o', "FILE", 0,  "Compile to FILE instead of immediate execution" },
  { 0 }
};

static error_t argpParser(int key, char *arg, struct argp_state *state);

// Our argp parser.
static struct argp argp_config = { argp_options, argpParser, argp_positional_doc, argp_doc };

/* Used by main to communicate with parse_opt. */
struct ArgResult
{
  char *romFilename;
  bool debugLog;
  //bool ast;
  //bool quiet;
  //char *output_file;
};

// argp callback to process a single option
static error_t argpParser(int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct ArgResult *args = (struct ArgResult *)state->input;

  switch (key)
    {
    case 'd':
      args->debugLog = true;
      break;
    /*
    case 'o':
      args->output_file = arg;
      break;
    */
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


int main(int argc, char **argv)
{
    struct ArgResult args;
    memset(&args, 0, sizeof(args));

    // parse args
    argp_parse(&argp_config, argc, argv, 0, 0, &args);

    //if(0 != args.output_file) {
    //  printf("OUTPUT FILE = %s\n", args.output_file);
    // }

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
    return 0;
}
