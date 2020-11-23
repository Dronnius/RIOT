//EXTRA SHELL COMMANDS

int dud_handler(int argc, char** argv); //defined in main.c
int extra_handler(int argc, char**argv); //defined in main.c

extern const shell_command_t shell_commands[];  //defined in main.c
extern shell_command_t _shell_command_list[];   //defined in sys/shell/commands/shell_commands.c

void check_extras(void);    //defined in main.c