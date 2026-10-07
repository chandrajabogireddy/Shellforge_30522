#include <ctype.h>

#include "lexer.h"

void lexer(const char *input, token_list_t *list)
{
    token_list_init(list);

    int i = 0;

    while (input[i] != '\0')
    {
        /* Skip whitespace */
        if (isspace((unsigned char)input[i]))
        {
            i++;
            continue;
        }

        /* Pipe | */
        if (input[i] == '|')
        {
            token_add(list, TOKEN_PIPE, "|");
            i++;
            continue;
        }

        /* Input redirection < */
        if (input[i] == '<')
        {
            token_add(list, TOKEN_INPUT, "<");
            i++;
            continue;
        }

        /* Output redirection > and append >> */
        if (input[i] == '>')
        {
            if (input[i + 1] == '>')
            {
                token_add(list, TOKEN_APPEND, ">>");
                i += 2;
            }
            else
            {
                token_add(list, TOKEN_OUTPUT, ">");
                i++;
            }

            continue;
        }

        /* Background & */
        if (input[i] == '&')
        {
            token_add(list, TOKEN_BACKGROUND, "&");
            i++;
            continue;
        }

        /* Normal word */
        char word[MAX_TOKEN_LEN];
        int j = 0;

        while (input[i] != '\0' &&
               !isspace((unsigned char)input[i]) &&
               input[i] != '|' &&
               input[i] != '<' &&
               input[i] != '>' &&
               input[i] != '&')
        {
            if (j < MAX_TOKEN_LEN - 1)
            {
                word[j++] = input[i];
            }

            i++;
        }

        word[j] = '\0';

        if (j > 0)
        {
            token_add(list, TOKEN_WORD, word);
        }
    }

    /* End of token list */
    token_add(list, TOKEN_END, "END");
}
