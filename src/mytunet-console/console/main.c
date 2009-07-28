#include <ctype.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


#include "../../os.h"
#include "../../des.h"
#include "../../md5.h"
#include "../../logs.h"
#include "../../tunet.h"
#include "../../dot1x.h"
#include "../../ethcard.h"
#include "../../userconfig.h"
#include "../../mytunet.h"
#include "../../util.h"
#include "../../setting.h"
#include "../../mytunetsvc.h"

static void sigint_handler(int signum) {
    static int exiting = 0;
    if (exiting)
    {
        printf("Already exiting, terminating.\n");
        exit(1);
    } else {
        printf("Logging out...\n");
        exiting = 1;
        mytunetsvc_set_stop_flag();
    }
}

VOID mytunetsvc_transmit_log_qt(VOID *xx)
{}

int main(int argc, char *argv[])
{

    INT bRunByUser = 0;
    CHAR *username, *password, *adapter;
    BOOL  usedot1x;
    INT   language, limitation;
    ETHCARD_INFO ethcards[16];
    INT     ethcardcount;
    INT     i;
    CHAR inputbuf[1024];

    USERCONFIG tmpTestConfig;

    mytunetsvc_set_transmit_log_func(MYTUNETSVC_TRANSMIT_LOG_POSIX);

    mytunetsvc_get_user_config(&tmpTestConfig);

    mytunetsvc_init();

    bRunByUser = (argc != 1);
    if(strlen(tmpTestConfig.szUsername) == 0) bRunByUser = 1;


    if(bRunByUser)
    {
        ethcardcount = get_ethcards(ethcards, sizeof(ethcards));
        if(argc <= 2)
        {
            puts( " MyTunet Service Program\n");

            puts(" Your Network Devices:");
            for(i = 0;i < ethcardcount; i++)
            {
                printf("    %d. %s\n", i, ethcards[i].desc);
            }
            puts("");
/*
            puts(
                    " Usage: mytunet set <adapter_index> <user> <pwd> <0/1 (need 802.1x?)>\n"
                    "                    <C/E (language)> <C/D/N (Campus/Domestic/NoLimit)>\n"
                    "   -- or -- \n"
                    "        mytunet set <adapter_index> <user> <0/1> <C/E> <C/D/N>\n"
                    "          (then you will input the password separetely.)\n"
                    "\n"
                    "  For example:  mytunet set 0 wang abcd 1 C D\n"
                   "   \n"
                   "    will set the logon information like this:\n"
                   "     - use the first network device,\n"
                   "     - username is \'wang\',\n"
                   "     - password is \'abcd\',\n"
                   "     - use 802.1x authorization(ONLY for Zijing 1# - 13# users),\n"
                   "     - language is Chinese,\n"
                   "     - open the connection for Domestic.\n"
                   "\n"
                   "  Another example: mytunet set 0 wang 1 C D\n"
                   "       then mytunet will prompt you to input the password.\n"
                   "\n"
                   " !!!==================  NOTICE  ===================!!!\n"
                   "  If you set your PASSWORD by 'mytunet set ...',\n"
                   "  you MUST run \'history -c\' to clean your command history,\n"
                   "  or your unencrypted password may leave in your system!!\n"
                   );
*/
            puts(
                    " Usage: mytunet set <adapter_index> <username> <0/1 (need 802.1x?)>\n"
                    "                    <C/E (language)> <C/D/N (Campus/Domestic/NoLimit)>\n"
                    "          (then you will input the password separetely.)\n"
                    "\n"
                    "  For example:  mytunet set 0 wang 1 C D\n"
                   "   \n"
                   "    will set the logon information like this:\n"
                   "     - use the first network device,\n"
                   "     - username is \'wang\',\n"
                   "     - use 802.1x authorization(ONLY for Zijing 1# - 13# users),\n"
                   "     - language is Chinese,\n"
                   "     - open the connection for Domestic.\n"
                   "\n"
                   "    then mytunet will prompt you to input the password.\n"
                   );
        }
        else
        {
            if(strcmp(argv[1], "set") == 0)
            {
                //if(argc != 8 && argc != 7)
                if(argc != 7)
                {
                    puts(" Wrong usage !");
                    return ERR;
                }

                i = 2;
                adapter = ethcards[atoi(argv[i++])].name;
                username = argv[i++];
                if(argc == 8)
                {
                    password = argv[i++];
                }
                else
                {
                    struct termios term, termsave;
                    char *p;

                    printf("Password:");
                    tcgetattr(STDIN_FILENO, &term);
                    tcgetattr(STDIN_FILENO, &termsave);
                    term.c_lflag &= ~(ECHO);
                    tcsetattr(STDIN_FILENO, TCSANOW, &term);
                    fgets(inputbuf, sizeof(inputbuf), stdin);
                    inputbuf[sizeof(inputbuf) - 1] = 0;
                    tcsetattr(STDIN_FILENO, TCSANOW, &termsave);

                    for(p = inputbuf + strlen(inputbuf) - 1; *p == '\n' || *p == '\r'; p--)
                        *p = 0;
                    puts("");

                    password = inputbuf;

                }
                usedot1x = atoi(argv[i++]);

                switch(tolower(argv[i++][0]))
                {
                    case 'e':
                        language = 0;
                        break;
                    case 'c':
                        language = 1;
                        break;
                    default:
                        language = 0;
                        break;

                }

                switch(tolower(argv[i++][0]))
                {
                    case 'c':
                        limitation = LIMITATION_CAMPUS;
                        break;
                    case 'd':
                        limitation = LIMITATION_DOMESTIC;
                        break;
                    case 'n':
                        limitation = LIMITATION_NONE;
                        break;
                    default:
                        limitation = LIMITATION_DOMESTIC;
                        break;
                }

                mytunetsvc_set_user_config(username, password, 0,
                                        adapter, limitation, language);
                mytunetsvc_set_user_config_dot1x(usedot1x, 0);

                return OK;
            }
            puts(" Wrong usage !");
        }

    }
    else
    {
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = sigint_handler;
        sigaction(SIGINT, &act, NULL);
        mytunetsvc_get_user_config(&g_UserConfig);
        mytunetsvc_main(0, NULL);
    }

    mytunetsvc_cleanup();
    return 0;
}
