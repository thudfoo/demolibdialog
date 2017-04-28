
#include <assert.h>
#include <dialog.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <wchar.h>
#include "demolibdialog.h"

#define MSGBOX 1
#define INFOBOX 0
#define INPUT 0
#define PASSWORD 1
#define LABEL 2
#define INPUTBOX 0
#define INSECURE 1
#define SECURE 0
#define CHOSEMENUITEM(thename) (strcmp(listitems[lastitem].name, thename) == 0)

int MAXLINES = 0, MAXCOLS = 0;
char *ourpath;
bool HAVECURSESW = FALSE; // See begin_trace()
int indent = 0;
char *indents[] =  
    {"  ", "    ", "      ", "        ", "          ", "            "}; 
const char *TRACE = "libdialog.trace";
char *color_names [] = {
    "BLACK", "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "CYAN", "WHITE"};
DIALOG_LISTITEM *ctmenuitems;
int nctitems;
DIALOG_COLORS *saved_color_table;


int
main(int argc, char *argv[])
{
    (void) argc;
    ourpath = argv[0];
    // Initialize colortable menu items
    nctitems = dlg_color_count();
    int ctsize = nctitems * sizeof(DIALOG_COLORS);
    saved_color_table = malloc(ctsize);
    memcpy(saved_color_table, dlg_color_table, ctsize);
    ctmenuitems = malloc(nctitems * sizeof(DIALOG_LISTITEM));
    int n;
    for(n=0; n < nctitems; n++)
    {
        ctmenuitems[n].name = strdup(dlg_color_table[n].name);
        ctmenuitems[n].text = NULL;
        ctmenuitems[n].help = strdup(dlg_color_table[n].comment);
    }
    addcolorattrs(); 

    DIALOG_LISTITEM listitems [] = {
        {"Calendar", "Demo of Calendar Widget", "", 0},
        {"Check", "Demo of Checklist Widget", "", 0},
        {"Colors", "Change the dialog colors...", "", 0},
        {"Editbox", "Demo of Editbox Widget", "", 0},
        {"Info", "Demo of Infobox Widget", "Widget is removed from screen after a few seconds", 0},
        {"Input", "Demo of Inputbox Widget", "", 0},
        {"InputMenu", "Demo of Inputmenu Widget", "", 0},
        {"Form", "Demo of Form Widget", "", 0},
        {"Gauge", "Demo of Gauge Widget", "", 0},
        {"Msgbox", "Demo of Msgbox Widget", "", 0},
        {"Pass", "Demo of Password Widget", "", 0},
        {"Pause", "Demo of Pause Widget", "", 0},
        {"Prgbox", "Demo of Prgbox Widget", "", 0},
        {"Progbox", "Demo of Progressbox Widget", "", 0},
        {"Radio", "Demo of Radiolist Widget", "", 0},
        {"Tail", "Demo of Tailbox Widget", "", 0},
        {"Trace", "Show contents of trace file", "", 0},
        {"UTF-8", "Demo dialogs with UTF-8...", "", 0},
        {"ViewFile", "Display a file using a textbox...", "", 0},
        {"YesNo", "Demo of YesNo Widget", "", 0},
    };

    memset(&dialog_state, 0, sizeof(dialog_state));
    dialog_state.use_scrollbar = TRUE;

    begin_trace();

    int nitems = sizeof(listitems)/sizeof(DIALOG_LISTITEM);
    static int lastitem = 1;
    int exit_status;
    FILE *tracefp;
    bool first = TRUE;
    while (1)
    {
        dlg_trace_msg("----------------- libdialog Demo Menu ------------------\n");
        begin_the_dialog();

        set_keep_window(FALSE);
        dialog_vars.default_item = listitems[lastitem].name;
        dialog_vars.item_help = TRUE;
        dialog_vars.ok_label = "Run";
        dialog_vars.cancel_label = "Quit";

        trace_libcall("dlg_menu");
        if (first)
        {
            first = FALSE;
            tracefp = dialog_state.trace_output;
        }
        else
            suspend_tracing(&tracefp);
        exit_status = dlg_menu("libdialog Demo Menu", "Choose:", 0, 50, nitems, nitems, listitems, &lastitem, 0);
        resume_tracing(&tracefp);

        trace_result(exit_status);
        clear_the_screen();
        end_the_dialog();
        if (exit_status != DLG_EXIT_OK) 
            break;

        dlg_trace_msg("\n----------------- %s ------------------\n", listitems[lastitem].name);
        if CHOSEMENUITEM("Trace")
            dialog_state.no_mouse = TRUE; // Stop Trace from ending when mouse action in its box
        begin_the_dialog();

        if CHOSEMENUITEM("Calendar") exit_status = Calendar();
        else if CHOSEMENUITEM("Check") exit_status = Checklist();
        else if CHOSEMENUITEM("Colors") exit_status = Colors();
        else if CHOSEMENUITEM("Editbox") exit_status = Editbox();
        else if CHOSEMENUITEM("Form") exit_status = Form();
        else if CHOSEMENUITEM("Gauge") exit_status = Gauge();
        else if CHOSEMENUITEM("Info") exit_status = Infobox();
        else if CHOSEMENUITEM("Input") exit_status = Inputbox();
        else if CHOSEMENUITEM("InputMenu") exit_status = Inputmenu();
        else if CHOSEMENUITEM("Msgbox") exit_status = Msgbox();
        else if CHOSEMENUITEM("Pass") exit_status = Password();
        else if CHOSEMENUITEM("Pause") exit_status = Pause();
        else if CHOSEMENUITEM("Prgbox") exit_status = Prgbox();
        else if CHOSEMENUITEM("Progbox") exit_status = Progressbox();
        else if CHOSEMENUITEM("Radio") exit_status = Radiolist();
        else if CHOSEMENUITEM("Tail") exit_status = Tailbox();
        else if CHOSEMENUITEM("Trace") exit_status = Tracefile();
        else if CHOSEMENUITEM("UTF-8") exit_status = Utf8();
        else if CHOSEMENUITEM("ViewFile") exit_status = Viewfile();
        else if CHOSEMENUITEM("YesNo") exit_status = Yesno();
        else error("Missing function for that Demo Menu item.");

        trace_result(exit_status);
        //trace_all_windows();
        delete_all_windows();
        end_the_dialog();
        clear_the_screen();
    }
    for (n=0; n<nctitems; n++)
        free(ctmenuitems[n].text);
    free(ctmenuitems);
    dlg_trace((const char *) 0); // close the trace file
    return EXIT_SUCCESS;
}

bool
is_regular_file(const char *filepath)
{
    struct stat statbuf;
    if (stat(filepath, &statbuf) == -1)
        return FALSE;
    return S_ISREG(statbuf.st_mode);
}

char *
get_formatted_str(const char *fmt,...)
{
    va_list ap;
    char *buffer = malloc(512);
    
    va_start(ap, fmt);
    (void) vsprintf(buffer, fmt, ap);
    va_end(ap);
    return buffer;
}

void
addcolorattrs(void)
{
    int n;
    for (n = 0; n < nctitems; n++)
    {
        free(ctmenuitems[n].text);
        ctmenuitems[n].text = (char *) calloc(80, sizeof(char));
        sprintf(ctmenuitems[n].text, "fg(%s)|bg(%s)|hilite(%s)",
                color_names[dlg_color_table[n].fg],
                color_names[dlg_color_table[n].bg],
                dlg_color_table[n].hilite ? "ON" : "OFF");
    }
}

void
reset_dialog_vars()
{
    memset(&dialog_vars, 0, sizeof(dialog_vars));
    trace_libcall("memset dialog_vars to 0");
}

void
trace_all_windows(void)
{
    dlg_trace_msg("trace_all_windows()\n");
    DIALOG_WINDOWS *p;

    for (p = dialog_state.all_windows; p != 0; p = p->next)
        dlg_trace_win(p->normal);
}

void
error(char *msg)
{
    dlg_trace_msg("error(\"%s\")\n", msg);
    dialog_msgbox("Error", msg, 0, 0, MSGBOX);
}

void
begin_the_dialog()
{
    trace_funentry("begin_the_dialog");
    reset_dialog_vars();
    dialog_vars.keep_tite = TRUE; // Must precede init_dialog to have affect
    trace_libcall("init_dialog");
    init_dialog(stdin, stderr);
    dialog_vars.backtitle = strdup(dialog_version());
    trace_libcall("dlg_put_backtitle");
    dlg_put_backtitle();
    if (!MAXLINES)
    {
        MAXLINES = SLINES;
        MAXCOLS = SCOLS;
        dlg_trace_msg("!!! SLINES: %d  SCOLS: %d\n", SLINES, SCOLS);
    }
    trace_funexit("begin_the_dialog");
}

void
begin_trace()
{
    // Start with an empty trace file
    FILE *fp = fopen(TRACE, "w");
    fclose(fp);
    dlg_trace(TRACE); //libdialog opens trace file and puts its FILE * in dialog_state.trace_output
    // show ldd of ourpath
    char tempfname[] = "/tmp/traceuniqoutXXXXXX";
    int fd;
    if ((fd = mkstemp(tempfname)) == -1)
    {
        error("mkstemp failed");
        return;
    }
    dlg_trace_msg("!!! output of ldd %s\n", ourpath);
    char cmd[256];
    sprintf(cmd, "ldd %s >%s", ourpath, tempfname);
    system(cmd);
    FILE *temp = fdopen(fd, "r");
    fgets(cmd, sizeof(cmd)-1, temp);
    while (!feof(temp))
    {
        cmd[0] = ' '; // Squash leading \t
        if (strstr(cmd, "libncursesw") != NULL)
            HAVECURSESW = TRUE;
        dlg_trace_msg("%s", cmd);
        fgets(cmd, sizeof(cmd)-1, temp);
    }
    close(fd);
}

void
beginyx(int y, int x)
{
    dialog_vars.begin_y = y;
    dialog_vars.begin_x = x;
    dialog_vars.begin_set = TRUE;
}

void
delete_all_windows(void)
{
    trace_funentry("delete_all_windows");

    set_keep_window(FALSE); // Need this so that dlg_del_window will do the right thing
    while (dialog_state.all_windows)
    {
        trace_libcall("dlg_del_window");
        dlg_del_window(dialog_state.all_windows->normal);
    }
    trace_funexit("delete_all_windows");
}

void
end_the_dialog()
{
    trace_funentry("end_the_dialog");
    set_no_mouse(FALSE);
    trace_libcall("end_dialog");
    end_dialog();
    trace_funexit("end_the_dialog");
}

const char *
exit_status_name(int exit_status)
{
    static const struct {
        int code;
        const char *name;
    } table[] = {
	{ DLG_EXIT_CANCEL, 	"DIALOG_CANCEL" },
	{ DLG_EXIT_ERROR,  	"DIALOG_ERROR" },
	{ DLG_EXIT_ESC,	   	"DIALOG_ESC" },
	{ DLG_EXIT_EXTRA,  	"DIALOG_EXTRA" },
	{ DLG_EXIT_HELP,   	"DIALOG_HELP" },
	{ DLG_EXIT_OK,	   	"DIALOG_OK" },
	{ DLG_EXIT_ITEM_HELP,	"DIALOG_ITEM_HELP" },
    };
    
    int n;
    for (n = 0; n < (int) (sizeof(table) / sizeof(table[0])); n++)
        if (table[n].code == exit_status)
           return table[n].name;
    return "UNKNOWN";
}

void
clear_the_screen()
{
    trace_funentry("clear_the_screen");
    trace_libcall("dlg_clear");
    dlg_clear();
    trace_libcall("dlg_put_backtitle");
    dlg_put_backtitle();
    trace_funexit("clear_the_screen");
}

void
create_condensed_trace_file(void)
{
    #define maxline 512
    /*
    char uniqfname[] = "/tmp/traceuniqoutXXXXXX";
    int fd;
    if ((fd = mkstemp(uniqfname)) == -1)
    {
        error("mkstemp failed");
        return strdup(infname);
    }
    //dlg_trace_msg("filename: %s\n", uniqfname);
    */
    FILE *uniqfile = fopen("condensed.trace", "w");
    FILE *infile = fopen(TRACE, "r");
    char buffer[maxline];
    char line[maxline] = "The file is empty\n";
    fgets(line, maxline, infile);
    int dupcnt = 0;
    fgets(buffer, maxline, infile);
    while(!feof(infile))
    {
        if (strcmp(line, buffer) == 0)
            dupcnt++;
        else
            {
                uniqput(uniqfile, line, dupcnt);
                dupcnt = 0;
                strcpy(line, buffer);
            }
        fgets(buffer, maxline, infile);
    }
    uniqput(uniqfile, line, dupcnt);
    fclose(infile);
    fclose(uniqfile);
}

void uniqput(FILE *fp, char *line, int dupcnt)
{
    if (strcmp(line, "rc:\n") != 0 && strncmp(line, "rc:#", 4) != 0)
    {
        fputs(line, fp);
        if (dupcnt == 1)
            fputs(line, fp);
        else if (dupcnt > 1)
            fprintf(fp, "+++ %d lines same as preceding line\n", dupcnt);
    }
}

int
safecall(int (*func) (void))
{
    DIALOG_VARS saved;
    dlg_save_vars(&saved);
    int exit_status = func();
    dlg_restore_vars(&saved);
    return exit_status;
}

int
choosecolor(char *cprompt)
{ // Return the color number chosen by user or -1
    trace_funentry("choosecolor");
    DIALOG_VARS saved;
    dlg_save_vars(&saved);
    static DIALOG_LISTITEM listitems [] = {
        {"BLACK", "\\Z0\\Zr_______________\\Zn", "", 0},
        {"RED", "\\Z1\\Zr_______________\\Zn", "", 0},
        {"GREEN", "\\Z2\\Zr_______________\\Zn", "", 0},
        {"YELLOW", "\\Z3\\Zr_______________\\Zn", "", 0},
        {"BLUE", "\\Z4\\Zr_______________\\Zn", "", 0},
        {"MAGENTA", "\\Z5\\Zr_______________\\Zn", "", 0},
        {"CYAN", "\\Z6\\Zr_______________\\Zn", "", 0},
        {"WHITE", "\\Z7\\Zr_______________\\Zn", "", 0},
    };
    static int nitems = sizeof(listitems)/sizeof(DIALOG_LISTITEM);
    static int lastitem = 0;
    dialog_vars.colors = TRUE;
    trace_libcall("dlg_menu");
    int exit_status = dlg_menu("Color Menu", cprompt, 0, 0, nitems, nitems, listitems, &lastitem, 0);
    dlg_restore_vars(&saved);
    trace_funexit("choosecolor");
    return (exit_status == DLG_EXIT_OK)?lastitem:-1;
}

int
choosecolortable(void)
{
    trace_funentry("choosecolortable");
    DIALOG_VARS saved;
    dlg_save_vars(&saved);
    // Make sure we have the latest colors in the menu
    addcolorattrs(); 

    static int lastitem = 0;

    dialog_vars.item_help = TRUE;
    dialog_vars.column_separator = "|";
    dialog_vars.default_item = ctmenuitems[lastitem].name;
    dialog_vars.ok_label = "Change";
    dialog_vars.cancel_label = "Done";

    trace_libcall("dlg_align_columns");
    dlg_align_columns(&ctmenuitems[0].text, sizeof(DIALOG_LISTITEM), nctitems);

    trace_libcall("dlg_menu");
    int exit_status = dlg_menu("Color Table Menu", "Choose:", 0, 0, 0, nctitems, ctmenuitems, &lastitem, 0);

    clear_the_screen();

    trace_funexit("choosecolortable");
    dlg_restore_vars(&saved);
    return (exit_status == DLG_EXIT_OK)?lastitem:-1;
}

int 
choosefilename(void)
{
    DIALOG_VARS saved;
    dlg_save_vars(&saved);
    set_keep_window(FALSE);
    char initial[FILENAME_MAX];
    if (getcwd(initial, FILENAME_MAX) != NULL)
        strcat(initial, "/");
    trace_libcall("dialog_fselect");
    int exit_status = dialog_fselect("Choose file", initial, 20, 80);
    clear_the_screen();
    trace_result(exit_status);
    dlg_restore_vars(&saved);
    return exit_status;
}

int
choosercfname(bool create, char *initial)
{
    DIALOG_VARS saved;
    dlg_save_vars(&saved);

    trace_libcall("dialog_fselect");
    int exit_status = dialog_fselect("Choose dialogrc file", initial, 20, 80);
    trace_result(exit_status);

    clear_the_screen();

    while (exit_status == DLG_EXIT_OK && create)
    {
        FILE *rcfile;
        if ((rcfile = fopen(dialog_vars.input_result, "r")) != NULL)
        {
            fclose(rcfile);
            dialog_vars.defaultno = TRUE;
            if ((exit_status = dialog_yesno("File Exists", "The file exists.\nOK to overwrite?",0,0)) == DLG_EXIT_CANCEL)
                break;
        }
        if ((rcfile = fopen(dialog_vars.input_result, "wt")) == NULL)
        {
            error("Failed to open for writing");
            exit_status = DLG_EXIT_CANCEL;
            break;
        }
        else
        {
            fclose(rcfile);
            break;
        }
    }
    dlg_restore_vars(&saved);
    return exit_status;
}

int
choosehilite(void)
{
    static DIALOG_LISTITEM listitems [] = {
        {"OFF", "Hilite OFF", "", 0},
        {"ON", "Hilite ON", "", 0},
    };
    static int nitems = sizeof(listitems)/sizeof(DIALOG_LISTITEM);
    static int lastitem = 0;
    int exit_status = dlg_menu("Color Menu", "Choose Hilite",
                               0, 0, nitems, /*menu height*/
                               nitems, listitems, &lastitem, 0);

    return (exit_status == DLG_EXIT_OK)?lastitem:-1;
}

int
choosesecure(void)
{
    static DIALOG_LISTITEM listitems [] = {
        {"SECURE", "Echo nothing for input", "", 0},
        {"INSECURE", "Echo * for each character in password", "", 0},
    };
    static int nitems = sizeof(listitems)/sizeof(DIALOG_LISTITEM);
    static int lastitem = 0;
    dialog_vars.default_item = listitems[lastitem].name;
    int exit_status = dlg_menu("Password Security Menu", "Choose:", 0, 0, nitems, nitems, listitems, &lastitem, 0);
    if (exit_status == DLG_EXIT_OK)
        return lastitem;
    else return -1;
}
 
int
dialogrcload(void)
{
    trace_funentry("dialogrcload");
    char initial[FILENAME_MAX];
    if (getcwd(initial, FILENAME_MAX) != NULL)
        strcat(initial, "/");

    int exit_status;
    while((exit_status =  choosercfname(FALSE, initial)) == DLG_EXIT_OK)
    {
        if (!is_regular_file(dialog_vars.input_result))
            error("The file you chose is not a regular file!");
        else
            break;
    }
    if (exit_status != DLG_EXIT_OK)
        exit_status = -1;
    else
    {
        if (setenv("DIALOGRC",dialog_vars.input_result, TRUE) != -1)
        {
            exit_status = dlg_parse_rc();
            if (exit_status == -1)
                error("Parse of rcfile failed!");
            unsetenv("DIALOGRC");
            clear_the_screen();
        }
    }
    dlg_clr_result();

    trace_funexit("dialogrcload");
    return exit_status;
}

void
change_colors(void)
{
    trace_libcall("dlg_color_setup");
    dlg_color_setup();
    trace_libcall("dlg_clear");
    dlg_clear();
}

int
colortableedit(void)
{
    trace_funentry("colortableedit");
    int ct = 0, fg = -1, bg = -1, hl = -1;

    while (ct != -1)
    {
        ct = safecall(&choosecolortable);
        if (ct != -1)
        {
            beginyx(5, 10);
            dialog_vars.default_item = color_names[dlg_color_table[ct].fg];
            char prompt[128];
            sprintf(prompt, "Choose foreground color for %s", dlg_color_table[ct].name);
            fg = choosecolor(prompt);
            if (fg != -1)
            {
                dialog_vars.begin_set = FALSE; // Center this dialog
                dialog_vars.default_item = color_names[dlg_color_table[ct].bg];
                bg = choosecolor("Choose background color");
                if (bg != -1)
                {
                    dialog_vars.default_item = dlg_color_table[ct].hilite?"ON":"OFF";
                    dlg_trace_msg("dialog_vars.default_item: %s\n", dialog_vars.default_item);
                    hl = choosehilite();
                }
            }
        }
        if (ct == -1 || fg == -1 || bg == -1 || hl == -1)    
            break;
        else {
                dlg_trace_msg("!!! User chose ct: %s fg: %d  bg: %d hl: %d\n", dlg_color_table[ct].name, fg, bg, hl);
                dlg_color_table[ct].fg = fg;
                dlg_color_table[ct].bg = bg;
                dlg_color_table[ct].hilite = hl;
                change_colors();
                /*
                clear_the_screen();
                end_the_dialog();
                begin_the_dialog();
                */
        }
    }
    clear_the_screen();
    trace_funexit("colortableedit");
    return DLG_EXIT_OK;
}

void
info(char *msg)
{
    dlg_trace_msg("info(\"%s\")\n", msg);
    dialog_msgbox("Info", msg, 0, 0, MSGBOX);
}

int
renamed_menutext(DIALOG_LISTITEM * items, int current, char *newtext)
{
    free(items[current].text);
    items[current].text = strdup(newtext);
    return DLG_EXIT_UNKNOWN;
}

void
set_dlg_clear_screen(bool value)
{
    dlg_trace_msg("=== dialog_vars.dlg_clear_screen = %s\n", value?"TRUE":"FALSE");
    dialog_vars.dlg_clear_screen = value;
} 

void
set_keep_window(bool value)
{
    dlg_trace_msg("=== dialog_vars.keep_window = %s\n", value?"TRUE":"FALSE");
    dialog_vars.keep_window = value;
} 

void
set_no_mouse(bool value)
{
    dlg_trace_msg("=== dialog_state.no_mouse = %s\n", value?"TRUE":"FALSE");
    dialog_state.no_mouse = value;
} 

void 
showfile(char *filename)
{
    int exit_status;
    char *title = get_formatted_str("dialog_textbox %s", filename);
    exit_status = dialog_textbox(title, filename, 0, 0);
    free(title);
    /*
    title = filename;
    char *cmd = get_formatted_str("dialog --trace dialog.trace --title %s --keep-tite --textbox %s 0 80",
                                  title, filename);
    exit_status = system(cmd);
    free(cmd);
    */
    clear_the_screen();
}

void
resume_tracing(FILE **savedfp)
{
    dialog_state.trace_output = *savedfp;
}

void 
suspend_tracing(FILE **savedfp)
{
    dlg_trace_msg("Tracing Suspended.\n");
    fflush(dialog_state.trace_output);
    *savedfp = dialog_state.trace_output;
    dialog_state.trace_output = NULL;
}

void
trace_result(int exit_status)
{
    dlg_trace_msg("!!! exit_status: %d (%s)\n", exit_status, exit_status_name(exit_status));
    if (dialog_vars.input_result != NULL)
        dlg_trace_msg("!!! result: %s\n", dialog_vars.input_result);
}

void
trace_funentry(char *funcname)
{
    assert(indent < sizeof(indents));
    dlg_trace_msg("((( %s%s() Entry\n", indents[++indent], funcname);
}

void
trace_funexit(char *funcname)
{
    dlg_trace_msg("))) %s%s() Exit\n", indents[indent--], funcname);
    assert(indent >= 0);
}

void
trace_libcall(char *funcname)
{
    dlg_trace_msg("    %s%s>>> %s()\n", indents[0], indents[indent], funcname);
}

/**************** Demos ****************/
int
Calendar(void)
{
    static int year = 0;
    static int month = 0;
    static int day = 0;
    dialog_vars.date_format = "%Y/%m/%d";
    dlg_trace_msg(">>> dialog_calendar()\n");
    int exit_status = dialog_calendar("Demo of Calendar Widget",
                                      "Select a date", 0, 0,
                                      day, month, year);
    if (exit_status == DLG_EXIT_OK) 
    {
        year = atoi(dialog_vars.input_result);
        month = atoi(dialog_vars.input_result+5);
        day = atoi(dialog_vars.input_result+8);
    }
    return exit_status;
}

int
Checklist(void)
{
    static DIALOG_LISTITEM listitems [] = {
        {"Apple", "Golden Delicious Are", "", 0},
        {"Orange", "Valencia", "", 0},
        {"Lime", "Good with Honeydew", "", 0},
        {"Grape", "Seedless", "", 0}
    };

    int nitems = sizeof(listitems)/sizeof(DIALOG_LISTITEM);
    static int lastitem = 0;
    dialog_vars.default_item = listitems[lastitem].name;

    trace_libcall("dlg_checklist");
    return  dlg_checklist("Demo of Checklist Widget",
                          "Checklist Prompt", 0, 50,
                          nitems, nitems, listitems,
                          " ABCDEFG",
                          FLAG_CHECK, &lastitem);
}

int
Colors(void)
{
    trace_funentry("colors");
    set_keep_window(FALSE);
    DIALOG_VARS saved;
    dlg_save_vars(&saved);
    int exit_status;


    DIALOG_LISTITEM listitems [] = {
        { "Edit", "Color Table", "A series of dialogs for changing|viewing color table entries", 0},
        { "Create", "DIALOGRC file", "User chooses filename first", 0},
        { "Load", "DIALOGRC file", "User chooses filename", 0},
        { "Restore", "color table", "Restore color table to state when this program began", 0},
    };
    static int nitems = sizeof(listitems)/sizeof(DIALOG_LISTITEM);
    static int lastitem = 0;

    while (TRUE)
    {
        dialog_vars.colors = TRUE;
        dialog_vars.cancel_label = "Done";
        trace_libcall("dlg_menu");
        exit_status = dlg_menu("Colors Menu", "Choose:", 0, 0, nitems, nitems, listitems, &lastitem, 0);
        dialog_vars.cancel_label = NULL;
        set_dlg_clear_screen(TRUE);
        if (exit_status != DLG_EXIT_OK)
            break;
        if CHOSEMENUITEM("Edit")
            safecall(&colortableedit);
        else if CHOSEMENUITEM("Create")
        {
            char initial[FILENAME_MAX];
            if (getcwd(initial, FILENAME_MAX) != NULL)
                strcat(initial, "/");
            if (choosercfname(TRUE, initial) == DLG_EXIT_OK)
            {
                dlg_create_rc(dialog_vars.input_result);
                trace_libcall("dialog_textbox");
                exit_status = dialog_textbox(dialog_vars.input_result, dialog_vars.input_result, 0, 0);
                clear_the_screen();
                trace_libcall("dlg_clr_result");
                dlg_clr_result();
            }
        }
        else if CHOSEMENUITEM("Load")
        {
                 exit_status = dialogrcload();
                 if (exit_status != DLG_EXIT_OK)
                     error("dialogrc load failed");
                 else
                 {
                     end_the_dialog();
                     begin_the_dialog();
                 }
        }
        else if CHOSEMENUITEM("Restore")
        {
            int ctsize = nctitems * sizeof(DIALOG_COLORS);
            memcpy(dlg_color_table, saved_color_table, ctsize);
        }
    }
    dlg_restore_vars(&saved);
    trace_funexit("colors");
    return DLG_EXIT_OK;
}

int
Editbox(void)
{
    char *title = "Demo of editbox Widget";
    static bool firstentry = TRUE;
    static int rows;
    static char **lines;
    if (firstentry)
    {
        firstentry = FALSE;
        rows = 3;
        lines = (void *) malloc((rows+1) * sizeof(char *));
        if (!lines)
        {
            error("editbox unable to allocate storage for lines!");
            return -1;
        }
        lines[0] = dlg_strclone("Here is the dead tree");
        lines[1] = dlg_strclone("Devoid of leaves");
        lines[2] = dlg_strclone("But a million stars");
        // Even though we told libdialog how many rows there are,
        // we must have a NULL to end the list.
        lines[3] = 0; 
    }
    beginyx(10, 2);
    dlg_trace_msg(">>> dlg_editbox()\n");
    return dlg_editbox(title, &lines, &rows, 20, 30);
}

int
Form(void)
{
    trace_funentry("Form");
    static DIALOG_FORMITEM listitems [6];

    static int formlineno = 1;
    static bool mustinitialize = TRUE;
    if (mustinitialize)
    {
        int listitemno = 0; 
        {listitems[listitemno].type = INPUT; // First Name
        listitems[listitemno].name = "First Name";
        listitems[listitemno].name_len = 11;
        listitems[listitemno].name_y = formlineno;
        listitems[listitemno].name_x = 1;
        listitems[listitemno].name_free = FALSE;
        listitems[listitemno].text = "";
        listitems[listitemno].text_len = 15;
        listitems[listitemno].text_y = formlineno;
        listitems[listitemno].text_x = 12;
        listitems[listitemno].text_flen = 15;
        listitems[listitemno].text_ilen = 15;
        listitems[listitemno].text_free = FALSE;
        listitems[listitemno].help = "Your first name";
        listitems[listitemno].help_free = FALSE;}
        listitemno++;
        {listitems[listitemno].type = INPUT; // Last Name
        listitems[listitemno].name = "Last Name";
        listitems[listitemno].name_len = 10;
        listitems[listitemno].name_y = formlineno;
        listitems[listitemno].name_x = 28;
        listitems[listitemno].name_free = FALSE;
        listitems[listitemno].text = "";
        listitems[listitemno].text_len = 15;
        listitems[listitemno].text_y = formlineno;
        listitems[listitemno].text_x = 39;
        listitems[listitemno].text_flen = 15;
        listitems[listitemno].text_ilen = 30;
        listitems[listitemno].text_free = FALSE;
        listitems[listitemno].help = "Your last name";
        listitems[listitemno].help_free = FALSE;}
        formlineno++;
        listitemno++;
        {listitems[listitemno].type = INPUT; // Username
        listitems[listitemno].name = "Email"; // Header
        listitems[listitemno].name_len = 6;
        listitems[listitemno].name_y = formlineno;
        listitems[listitemno].name_x = 1;
        listitems[listitemno].name_free = FALSE;
        listitems[listitemno].text = ""; // Field
        listitems[listitemno].text_len = 0; // not operative?
        listitems[listitemno].text_y = formlineno;
        listitems[listitemno].text_x = 7; // Field text starts in column 7
        listitems[listitemno].text_flen = 15; // Field screen length (shows flen - 1 chars + cursor)
        listitems[listitemno].text_ilen = 30; // Field can input max chars
        listitems[listitemno].text_free = FALSE;
        listitems[listitemno].help = "Your email username";
        listitems[listitemno].help_free = FALSE;}
        listitemno++;
        {listitems[listitemno].type = LABEL; // @
        listitems[listitemno].name = "@"; // Header
        listitems[listitemno].name_len = 1;
        listitems[listitemno].name_y = formlineno;
        listitems[listitemno].name_x = 22;
        listitems[listitemno].name_free = FALSE;
        listitems[listitemno].text = ""; // Field
        listitems[listitemno].text_len = 0; // not operative?
        listitems[listitemno].text_y = formlineno;
        listitems[listitemno].text_x = 23; // Field text start column
        listitems[listitemno].text_flen = -1; // Field screen length (shows flen - 1 chars + cursor)
        listitems[listitemno].text_ilen = 0; // Field can input max chars
        listitems[listitemno].text_free = FALSE;
        listitems[listitemno].help = "";
        listitems[listitemno].help_free = FALSE;}
        listitemno++;
        {listitems[listitemno].type = INPUT; // email domain 
        listitems[listitemno].name = ""; // Header
        listitems[listitemno].name_len = 0;
        listitems[listitemno].name_y = formlineno;
        listitems[listitemno].name_x = 23;
        listitems[listitemno].name_free = FALSE;
        listitems[listitemno].text = ""; // Field
        listitems[listitemno].text_len = 0; // not operative?
        listitems[listitemno].text_y = formlineno;
        listitems[listitemno].text_x = 24; // Field text start column
        listitems[listitemno].text_flen = 30; // Field screen length (shows flen - 1 chars + cursor)
        listitems[listitemno].text_ilen = 30; // Field can input max chars
        listitems[listitemno].text_free = FALSE;
        listitems[listitemno].help = "Your email server's domain name";
        listitems[listitemno].help_free = FALSE;}
        formlineno++;
        listitemno++;
        {listitems[listitemno].type = INPUT; // Country
        listitems[listitemno].name = "Country"; // Header
        listitems[listitemno].name_len = 7;
        listitems[listitemno].name_y = formlineno;
        listitems[listitemno].name_x = 41;
        listitems[listitemno].name_free = FALSE;
        listitems[listitemno].text = "USA"; // Field
        listitems[listitemno].text_len = 30; // not operative?
        listitems[listitemno].text_y = formlineno;
        listitems[listitemno].text_x = 1; // Field text starts in column 7
        listitems[listitemno].text_flen = 30; // Field screen length (shows flen - 1 chars + cursor)
        listitems[listitemno].text_ilen = 30; // Field can input max chars
        listitems[listitemno].text_free = FALSE;
        listitems[listitemno].help = "Your country";
        listitems[listitemno].help_free = FALSE;}
        mustinitialize = FALSE;
    }
    int nitems = sizeof(listitems)/sizeof(DIALOG_FORMITEM);

    static int lastitem = 0;

    dialog_vars.help_button = TRUE;
    dialog_vars.item_help = TRUE;
    dialog_vars.default_item = listitems[lastitem].name;

    int exit_status = dlg_form("Demo Form via dlg_form", "Change:", 0, 80, formlineno+1, nitems, listitems,
                               &lastitem);

    trace_funexit("Form");
    return exit_status;
}

int
Gauge(void)
{
    trace_funentry("gauge");
    set_keep_window(TRUE); // Absolutely neccesary!
    beginyx(10, 10);
    char *prompt = "Do not press keys. Gauge doesn't process keystrokes: they go to next dialog, if any.";
    trace_libcall("dlg_allocate_gauge");
    void *objptr = dlg_allocate_gauge("Gauge Title", prompt, 0, 0, 0);
    dlg_update_gauge(objptr, 0);
    sleep(2);
    dlg_update_gauge(objptr, 30);
    sleep(2);
    dlg_update_gauge(objptr, 90);
    sleep(2);
    dlg_update_gauge(objptr, 100);
    trace_libcall("dlg_free_gauge");
    dlg_trace_win((*dialog_state.all_windows).normal);

    dialog_vars.begin_set = 0; // Center the following msgbox
    char *msg = "The gauge widget should still be visible on the screen.\n"
                "After you dismiss this msgbox, both dialogs will disappear.";
    trace_libcall("dialog_msgbox");
    int exit_status = dialog_msgbox("msgbox Title", msg, 0, 0, MSGBOX);
    trace_all_windows();
    // If dlg_free_gauge runs before the dialog_msgbox, the msgbox will not appear on screen
    // but it will appear in the trace and with an exit_status of DIALOG_ESC
    dlg_free_gauge(objptr); 
    trace_funexit("gauge");
    return exit_status;
}

int
Infobox(void)
{
    beginyx(20, 10);
    dialog_vars.help_line = "Just wait a moment...";
    // An infobox has no items, but the following works!
    dialog_vars.item_help = TRUE;
    dlg_item_help("An item_help line for a widget without items!");
    dlg_trace_msg(">>> dialog_msgbox(INFOBOX)\n");

    int exit_status = dialog_msgbox("Demo of Infobox",
                                    "Here is the info: Notice the itemhelp line at the bottom of the Screen!\n",
                                    0, 0, INFOBOX);
    sleep(5);
    return exit_status;
}

int
Inputbox(void)
{
    static char *initial = "change me (hint: ^u)";
    dlg_trace_msg(">>> dialog_inputbox()\n");
    int exit_status = dialog_inputbox("Demo Inputbox Widget", "Enter your input:", 0, 0, initial, INPUTBOX);
    if (exit_status == DLG_EXIT_OK)
        initial = dialog_vars.input_result;
    return exit_status;
}
    
int
Inputmenu(void)
{
    trace_funentry("Inputmenu");
    static DIALOG_LISTITEM listitems [] = {
        {"Username", "", "The name of the User", 0},
        {"PIN", "", "", 0},
        {"Zipcode", "", "", 0},
    };
    static bool mustinitialize = TRUE;
    if (mustinitialize)
    {
        // initialize here so renamed_menutext can free witn impunity
        listitems[0].text = strdup("S Claus");
        listitems[1].text = strdup("");
        listitems[2].text = strdup("99141");
        mustinitialize = FALSE;
    }

    int nitems = sizeof(listitems)/sizeof(DIALOG_LISTITEM);

    static int lastitem = 0;

    dialog_vars.input_menu = TRUE;
    dialog_vars.help_button = TRUE;
    dialog_vars.item_help = TRUE;
    // Using libdialog means no automatic extra_button
    dialog_vars.extra_button = TRUE;
    dialog_vars.extra_label = "Change";
    dialog_vars.default_item = listitems[lastitem].name;

    int exit_status = dlg_menu("Demo Inputmenu via dlg_menu", "Change:", 0, 50, nitems*3, nitems, listitems, &lastitem, &renamed_menutext);

    //free(dialog_vars.input_result); // Do not want tag names accumulating in input_result
    //dialog_vars.input_result = '\0';
    dlg_clr_result();
    trace_funexit("inputmenu");
    return exit_status;
}

int
Msgbox(void)
{
    dialog_vars.help_line = "Notice the item help at bottom of screen!\n";
    // A msgbox has no items, but the following works!
    dialog_vars.item_help = TRUE;
    dialog_vars.extra_button = TRUE;
    dialog_vars.help_button = TRUE;
    dialog_vars.default_button = DLG_EXIT_HELP;
    dlg_item_help("An item_help line for a widget without items!");
    dlg_trace_msg(">>> dialog_msgbox(MSGBOX)\n");

    return dialog_msgbox("Demo of Msgbox",
                                    "Here is the msg:\n"
                                    "It has a helpline on the bottom of the box.\n"
                                    "And optional Extra and Help buttons",
                                    0, 0, MSGBOX);
}

int
Password(void)
{
    int security = safecall(&choosesecure);
    if (security == -1)
        return DLG_EXIT_CANCEL;
    dialog_vars.insecure = security;
    dlg_trace_msg(">>> dialog_inputbox()\n");
    int exit_status = dialog_inputbox("Demo Inputbox Widget", "Enter your password:", 0, 0, "", PASSWORD);
    if (exit_status == DLG_EXIT_OK)
        dialog_msgbox("Result of Password", dialog_vars.input_result, 0, 0, MSGBOX);
    return exit_status;
}

int 
Pause(void)
{
    trace_funentry("pause");
    set_keep_window(TRUE);
    char *title = "Demo of Pause Widget";
    char *prompt = "Watch the countdown.";
    trace_libcall("dialog_pause");
    trace_funexit("pause");
    return dialog_pause(title, prompt, 10, 0, 5);
}

int 
Prgbox(void)
{
        trace_funentry("prgbox");
    int exit_status = dialog_inputbox("Get prgbox command", "Enter your command:", 0, 0, "w", INPUTBOX);
    if (exit_status != DLG_EXIT_OK)
        return exit_status;
    if (strlen(dialog_vars.input_result) == 0 || dialog_vars.input_result == NULL)
        dlg_add_result("w");
    dialog_vars.keep_tite = TRUE;
    char *title = "Demo of Prgbox Widget";
    char *prompt;
    char *command;
    command = get_formatted_str("command.sh %s", dialog_vars.input_result);
    prompt = get_formatted_str("Output of command: %s", dialog_vars.input_result);
        trace_libcall("dialog_prgbox");
    exit_status = dialog_prgbox(title, prompt, command, 25, 80, TRUE);
        trace_funexit("prgbox");
    free(prompt);
    free (command);
    return exit_status;
}

int 
Progressbox(void)
{
        trace_funentry("progressbox");
    pid_t pid;
    int mypipe[2];
    int exit_status;

    /* Create the pipe. */
    if (pipe (mypipe))
     {
       error("Pipe Failed!");
       return EXIT_FAILURE;
     }

    /* Create the child process. */
    pid = fork ();
    if (pid == (pid_t) 0)
    {
        /* This is the CHILD process.
          Close other end first. */
        close (mypipe[1]);
        char *title = "Demo of Progressbox Widget";
        char *prompt = "Prompt Text";
            trace_libcall("dlg_progressbox");
        begin_the_dialog();
        FILE *pipefp = fdopen(mypipe[0], "r");
        exit_status = dlg_progressbox(title, prompt, 25, 80, FALSE, pipefp );
        //end_the_dialog();
        info("Progress has ended!");
        _exit(exit_status);
    }
    else if (pid < (pid_t) 0)
    {
       error("Fork Failed!");
       return EXIT_FAILURE;
    }
    else
    {
       /* This is the PARENT process.
          Close other end first. */
       close (mypipe[0]);
       FILE *stream = fdopen(mypipe[1], "w");
       fprintf(stream, "Hello Libdialog!\n");
       fflush(stream);
       sleep(2);
       fprintf(stream, "Hello again Libdialog!\n");
       fflush(stream);
       sleep(2);
       fprintf(stream, "End of Progressbox\n");
       fflush(stream);
       sleep(2);
       fclose(stream);
       wait(&exit_status);
       char *msg = get_formatted_str("Child exit_status: %d", exit_status);
       info(msg);
       free(msg);
    }
    return DLG_EXIT_OK;
}

int
Radiolist(void)
{
    static DIALOG_LISTITEM listitems [] = {
        {"Python", "Grrrreat!", "", 0},
        {"C++", "Better C, maybe", "", 0},
        {"Ruby", "Yet another", "", 0},
    };

    int nitems = sizeof(listitems)/sizeof(DIALOG_LISTITEM);
    static int lastitem = 0;
    dialog_vars.default_item = listitems[lastitem].name;

    dlg_trace_msg(">>> dlg_checklist(FLAG_RADIO)\n");
    return  dlg_checklist("Demo of Radiolist Widget", "Choose just one by toggling with Spacebar",
            0, 50, nitems, nitems, listitems, " ABCDEFG", FLAG_RADIO, &lastitem);
}

int
Tailbox(void)
{
    char *msg = "User should immediately press Enter when tailbox appears.\n"
                "Otherwise trace file will quickly expand with 100s of lines.\n"
                "When OK in tailbox is pressed, Demo application will terminate.\n\n"
                "Do you still want to run tailbox?";
    dialog_vars.defaultno = TRUE;
    dlg_trace_msg(">>> dialog_yesno()\n");
    int exit_status = dialog_yesno("Are you sure?", msg, 0, 0);
    trace_result(exit_status);
    if (exit_status == DLG_EXIT_OK)
    {
        beginyx(3, 0);
        trace_libcall("dialog_tailbox");
        exit_status = dialog_tailbox("Tail of the trace file", TRACE, 10, 60, FALSE);
    }
    return exit_status;
}
    
int
Tracefile(void)
{
    trace_funentry("tracefile");

    set_keep_window(FALSE);
    dialog_vars.exit_label = "Quit";

    fflush(dialog_state.trace_output);
    create_condensed_trace_file();

    trace_libcall("dialog_textbox");
    FILE *savedfp;
    suspend_tracing(&savedfp);
    int exit_status = dialog_textbox("Contents of trace file", "condensed.trace", 0, 0);
    resume_tracing(&savedfp);

    trace_funexit("tracefile");
    return exit_status;
}

int
Utf8(void)
{
    if (!HAVECURSESW)
    {
        error("It appears your libdialog was built without libncursesw, so you cannot run utf-8 dialogs.");
        return DLG_EXIT_CANCEL;
    }

    int exit_status;
    int saved_scr_fg = dlg_color_table[0].fg, saved_scr_bg = dlg_color_table[0].bg, saved_scr_hilite = dlg_color_table[0].hilite;
    dlg_color_table[0].fg = COLOR_WHITE;
    dlg_color_table[0].bg = COLOR_GREEN;
    dlg_color_table[0].hilite = 1; // ON
    change_colors();
    
    dialog_vars.begin_set = TRUE;
    dialog_vars.begin_y = 2; 
    char *saved_backtitle = dialog_vars.backtitle; 
    dialog_vars.backtitle = "full";
    dlg_put_backtitle();        
    showfile("data/fullwidthchars.txt");
    dialog_vars.backtitle = "half";
    dlg_put_backtitle();        
    showfile("data/halfwidthchars.txt");
    dialog_vars.backtitle = "dingbats";
    dlg_put_backtitle();        
    showfile("data/dingbats.txt");
    dlg_clear(); // Remove backtitle text and its line from screen
    showfile("data/cardsuits.txt");
    dialog_vars.backtitle = saved_backtitle;
    dlg_put_backtitle();        
    dialog_vars.begin_set = FALSE;
    
    char *msg = "ＡＢＣＤＥＦＧＨＩＪＫＬ♣℅"; 
    char *xmsg = "ＡＢＣＤＥＦＧＨＩＪＫＬ ♣℅ ascii"; 
    exit_status = dialog_msgbox("ＣＡＬＬＩＮＧ dialog_msgbox",xmsg, 0, 0, MSGBOX);
    char *cmd = get_formatted_str("dialog --title \"Ｒunning dialog --msgbox\" --keep-tite --msgbox \"%s\" 0 0", msg);
    exit_status = system(cmd);
    dlg_color_table[0].fg = saved_scr_fg;
    dlg_color_table[0].bg = saved_scr_bg;
    dlg_color_table[0].hilite = saved_scr_hilite;
    free(cmd);
    return exit_status;
}

int
Viewfile(void)
{
    int exit_status = choosefilename();
    if (exit_status ==  DLG_EXIT_OK)
        showfile(dialog_vars.input_result);
    return DLG_EXIT_OK;
}

int 
Yesno(void)
{
        trace_funentry("yesno");
    dialog_vars.colors = TRUE;
    char *title = "Demo of YesNo Widget";
    char *prompt = "put some \\Zbcolor\\ZB coding here";
        trace_libcall("dialog_yesno");
    int exit_status = dialog_yesno(title, prompt, 0, 0);
        trace_funexit("yesno");
    return exit_status;
}
