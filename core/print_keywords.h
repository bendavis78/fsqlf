#ifndef print_keywords_h
#define print_keywords_h

#include "settings.h"
#include "global_variables.h"
#include "create_conf_file.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>



// tab_string is needed to be able switch between spaces "    " and tabs '\t'
char * tab_string = "    ";



inline int max(int a, int b){
    return a > b ? a : b;
}

#define KW_FUNCT_ARRAY_SIZE (3)
#define CONFIG_FILE "formatting.conf"

// struture to store text keyword text  space,tab,newline, function to execute  before/after printig the keyword
typedef struct{
    int new_line;
    int indent;
    int space;
} spacing_counts;


typedef struct{
    spacing_counts before;
    spacing_counts after;

    int print_original_text;
    int print_case;
    char * text;

    int (*funct_before[KW_FUNCT_ARRAY_SIZE])();
    int (*funct_after [KW_FUNCT_ARRAY_SIZE])();
} t_kw_settings;



void debug_kw_settings(t_kw_settings s){
    extern FILE * yyout;
    fprintf(yyout,"\nspace_before %d , tab_before %d , nl_before %d , space_after %d , tab_after %d , nl_after %d\n , text %s "
           ,s.before.space, s.before.indent, s.before.new_line, s.after.space, s.after.indent, s.after.new_line, s.text);
    //printf("after %X %X %X\n", s.funct_after[0],s.funct_after[1],s.funct_after[2]);//debug string
    //printf("before %X %X %X\n", s.funct_before[0],s.funct_before[1],s.funct_before[2]);//debug string
}



int debug_p();// TODO : make separate .c and .h files




static spacing_counts combine_spacings(
    spacing_counts afterspacing_of_prev
    ,spacing_counts beforespacing_of_current
    ,int global_indent_level
){
    spacing_counts combined;
    combined.new_line   = afterspacing_of_prev.new_line + beforespacing_of_current.new_line;

    if(!beforespacing_of_current.new_line){
        combined.indent     = max(afterspacing_of_prev.indent, beforespacing_of_current.indent);
        combined.indent     += combined.new_line ? global_indent_level : 0;
        combined.space      = max(afterspacing_of_prev.space, beforespacing_of_current.space);
    }
    else{
        combined.indent     = beforespacing_of_current.indent;
        combined.indent     += combined.new_line ? global_indent_level : 0;
        combined.space      = beforespacing_of_current.space;
    }
    return combined;
}


static void print_nlines(FILE * yyout, int count){
    int i;
    for(i=0; i < count; i++) fputs("\n", yyout);
}
static void print_tabs(FILE * yyout, int count){
    int i;
    for(i=0; i < count; i++) fputs(tab_string, yyout);
}
static void print_spaces(FILE * yyout, int count){
    int i;
    for(i=0; i < count; i++) fputs(" ", yyout);
}

static void print_spacing(FILE * yyout, t_kw_settings s, int global_indent_level){
/* Prints all spacing of the program
 * Except when space is inside the multiword-keyword (e.g. "LEFT JOIN"), those will not be printed by this function
 * 'spacing' means new lines, tabs and spaces
*/
    static spacing_counts from_previous = {0,0,0}; // keep track of 'after' spacing from previous call
    spacing_counts combined = {0,0,0}; // it will hold spacing counts that will actualy be printed

    if(s.text[0]!='\0'){
        // Calculate what spacings will be printed
        combined = combine_spacings(from_previous, s.before, global_indent_level);

        // Print
        print_nlines(yyout, combined.new_line);
        print_tabs(yyout, combined.indent); // FIXME-prevspacesprinted: can lead to spaces preceeding tab char
        print_spaces(yyout, combined.space);

        // Save settings for next function call - overwrite
        from_previous = s.after;
    }
    else{
        // this 'else' (actualy all 'if-else') is workaround for printing space, from echo_print()
        // Save settings for next function call - not overwrite, but combine them somehow.
        from_previous.new_line  += s.after.new_line;
        from_previous.indent    += s.after.indent;
        from_previous.space     = max(s.after.space, from_previous.space);
    }
}


#define MAX_KEYWORD_SIZE (50)
enum{CASE_none,CASE_lower,CASE_UPPER,CASE_Initcap};
char* stocase(char* s_text, int s_case){
    static char formatted_result[MAX_KEYWORD_SIZE];
    int i;

    switch(s_case){
        case CASE_lower:
            for(i=0; i<strlen(s_text); i++) formatted_result[i] = tolower(s_text[i]);
            break;
        case CASE_UPPER:
            for(i=0; i<strlen(s_text); i++) formatted_result[i] = toupper(s_text[i]);
            break;
        case CASE_Initcap:
            formatted_result[0] = toupper(s_text[0]);
            for(i=1; i<strlen(s_text); i++) formatted_result[i] = tolower(s_text[i]);
            break;
        case CASE_none:
            return s_text;
    }
    formatted_result[strlen(s_text)]='\0';
    return formatted_result;
}

void kw_print(FILE * yyout, char * yytext, t_kw_settings s){
    int i=0;
    // call keyword specific functions. Before fprintf
    for(i=0; i < KW_FUNCT_ARRAY_SIZE && s.funct_before[i] != NULL ; i++)
        s.funct_before[i]();

    print_spacing(yyout, s, currindent); // print spacing before keyword

    fprintf(yyout,"%s",stocase( s.print_original_text ? yytext : s.text , s.print_case)); // 1st deside what text to use (original or degault), then handle its case

    // call keyword specific functions. After fprintf
    for(i=0; i < KW_FUNCT_ARRAY_SIZE && s.funct_after[i] != NULL ; i++)
        s.funct_after[i]();
}



void echo_print(FILE * yyout, char * txt){
    int i=0, space_cnt=0, nl_cnt=0, length, nbr;

    t_kw_settings s;
    s.before.new_line=s.before.indent=s.before.space=s.after.new_line=s.after.indent=s.after.space=0;

    //count blank characters at the end of the text
    length = strlen(txt);
    for(i=length-1; txt[i]==' '  && i>=0; i--) space_cnt++;
    for(          ; txt[i]=='\n' && i>=0; i--) nl_cnt++; // 'i=..' omited to continue from where last loop has finished

    // Prepare text for print (i is used with value set by last loop)
    nbr=i+1;
    s.text = (char*) malloc((nbr+1)*sizeof(char));
    strncpy(s.text, txt, nbr);
    s.text[nbr]='\0';

    // Spacing
    s.after.new_line = nl_cnt;
    s.after.space = space_cnt;
    print_spacing(yyout, s, currindent);

    // Print
    fprintf(yyout,"%s",s.text);
    free(s.text);
}


// initialize t_kw_settings variables
#define T_KW_SETTINGS_MACRO( NAME , ... ) \
    t_kw_settings NAME ;
#include "t_kw_settings_list.def"
#undef T_KW_SETTINGS_MACRO



void set_case(int keyword_case){
    #define T_KW_SETTINGS_MACRO( NAME , ... ) \
        NAME.print_case = keyword_case;
    #include "t_kw_settings_list.def"
    #undef T_KW_SETTINGS_MACRO
}


void set_text_original(int ind_original){
    #define T_KW_SETTINGS_MACRO( NAME , ... ) \
        NAME.print_original_text = ind_original;
    #include "t_kw_settings_list.def"
    #undef T_KW_SETTINGS_MACRO
}

void init_all_settings(){
    #define T_KW_SETTINGS_MACRO( NAME,nlb,tb,sb,nla,ta,sa,TEXT , fb1,fb2,fb3,fa1,fa2,fa3) \
        NAME.before.new_line    = nlb;    \
        NAME.before.indent      = tb;     \
        NAME.before.space       = sb;     \
                                    \
        NAME.after.new_line     = nla;    \
        NAME.after.indent       = ta;     \
        NAME.after.space        = sa;     \
        NAME.print_original_text = 0; \
        NAME.print_case         = CASE_UPPER; \
        NAME.text               = TEXT;   \
                                    \
        NAME.funct_before[0] = fb1; \
        NAME.funct_before[1] = fb2; \
        NAME.funct_before[2] = fb3; \
        NAME.funct_after [0] = fa1; \
        NAME.funct_after [1] = fa2; \
        NAME.funct_after [2] = fa3;
    #include "t_kw_settings_list.def"
    #undef T_KW_SETTINGS_MACRO
}




#define BUFFER_SIZE (100)
#define VALUE_NUMBER (6)

int setting_value(char * setting_name, int * setting_values)
{
    #define T_KW_SETTINGS_MACRO( NAME, ... )    \
    if( strcmp(#NAME,setting_name) == 0 ){      \
        NAME.before.new_line    = setting_values[0];  \
        NAME.before.indent      = setting_values[1];  \
        NAME.before.space       = setting_values[2];  \
        NAME.after.new_line     = setting_values[3];  \
        NAME.after.indent       = setting_values[4];  \
        NAME.after.space        = setting_values[5];  \
    }
    #include "t_kw_settings_list.def"
    #undef T_KW_SETTINGS_MACRO
}







#include <stdlib.h>
#include <string.h>

int read_configs()
{
    FILE * config_file;
    char line[BUFFER_SIZE+1] , setting_name[BUFFER_SIZE+1];
    int setting_values[VALUE_NUMBER];
    char * chr_ptr1;
    int i;

    config_file=fopen(CONFIG_FILE,"r"); // open file in working directory
    #ifndef _WIN32
    if(config_file == NULL)
    {
        // in non-windows (probably unix/linux) also try folder in user-home directory
        #define PATH_STRING_MAX_SIZE (200)
        char full_path[PATH_STRING_MAX_SIZE+1];
        strncpy(full_path, getenv("HOME") , PATH_STRING_MAX_SIZE);
        strncat(full_path, "/.fsqlf/" CONFIG_FILE ,PATH_STRING_MAX_SIZE - strlen(full_path));
        config_file=fopen(full_path,"r");
    }
    #endif

    if(config_file == NULL)
    {
        return 1;
    }

    while( fgets( line, BUFFER_SIZE, config_file ) )
    {
        if(line[0]=='#') continue; // lines starting with '#' are commnets
        // find and mark with '\0' where first stace is (end c string)
        if( !(chr_ptr1=strchr(line,' ')) ) continue;
        chr_ptr1[0]='\0';

        // store into variables
        strncpy( setting_name, line, BUFFER_SIZE );
        for(i = 0; i < VALUE_NUMBER; i++){
            setting_values[i] = strtol( chr_ptr1+1, &chr_ptr1, 10 );
        }

        // debug        printf("\nsetting_name='%s'; v0='%d'; v1='%d'; v2='%d'; v3='%d'; v4='%d'; v5='%d';",setting_name, setting_values[0], setting_values[1], setting_values[2], setting_values[3], setting_values[4], setting_values[5]);
        setting_value(setting_name,setting_values);
    }

    fclose(config_file);
    return 0;
}




#endif
