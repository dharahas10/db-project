/************************************************************
	Project#1:	CLP & DDL
 ************************************************************/

#include "db.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>
#include <map>
#include <sstream>

#if defined(_WIN32) || defined(_WIN64)
#define strcasecmp _stricmp
#endif

struct cmp_str {
    bool operator()(char const *a, char const *b) const {
        return strcmp(a, b) < 0;
    }
};

bool debug = false;

int main(int argc, char **argv) {
    int rc = 0;
    token_list *tok_list = NULL, *tok_ptr = NULL, *tmp_tok_ptr = NULL;

    if ((argc != 2) || (strlen(argv[1]) == 0)) {
        printf("Usage: db \"command statement\"\n");
        return 1;
    }

    rc = execute_statement(argv[1]);

    return rc;
}

int execute_statement(char *statement) {
    token_list *tok_list = NULL, *tok_ptr = NULL;
    int rc = initialize_tpd_list();

    // check if tpd is initialized
    if (rc) {
        printf("\nError in initialize_tpd_list().\nrc = %d\n", rc);
        return rc;
    }

    // parse statement for token
    rc = get_token(statement, &tok_list);
    if (rc) {
        printf("\nError in parsing statement to tokens.\nrc = %d\n", rc);
        free_token_list(tok_list);
        return rc;
    }

    // print tokens
    if (debug) {
        tok_ptr = tok_list;
        while (tok_ptr != NULL) {
            printf("%16s \t%d \t %d\n", tok_ptr->tok_string, tok_ptr->tok_class,
                   tok_ptr->tok_value);
            tok_ptr = tok_ptr->next;
        }
    }

    // semantics
    rc = do_semantic(tok_list);

    if (rc) {
        tok_ptr = tok_list;
        while (tok_ptr != NULL) {
            if ((tok_ptr->tok_class == error) ||
                (tok_ptr->tok_value == INVALID)) {
                printf("\nError in the string: %s\n", tok_ptr->tok_string);
                printf("rc=%d\n", rc);
                break;
            }
            tok_ptr = tok_ptr->next;
        }
    }

    free_token_list(tok_list);
    return rc;
}

void free_token_list(token_list *t_list) {
    token_list *tok_ptr = t_list;
    token_list *tmp_tok_ptr = NULL;
    while (tok_ptr != NULL) {
        tmp_tok_ptr = tok_ptr->next;
        free(tok_ptr);
        tok_ptr = tmp_tok_ptr;
    }
}

/************************************************************* 
	This is a lexical analyzer for simple SQL statements
 *************************************************************/
int get_token(char *command, token_list **tok_list) {
    int rc = 0, i, j;
    char *start, *cur, temp_string[MAX_TOK_LEN];
    bool done = false;

    start = cur = command;
    while (!done) {
        bool found_keyword = false;

        /* This is the TOP Level for each token */
        memset((void *)temp_string, '\0', MAX_TOK_LEN);
        i = 0;

        /* Get rid of all the leading blanks */
        while (*cur == ' ')
            cur++;

        if (cur && isalpha(*cur)) {
            // find valid identifier
            int t_class;
            do {
                temp_string[i++] = *cur++;
            } while ((isalnum(*cur)) || (*cur == '_'));

            if (!(strchr(STRING_BREAK, *cur))) {
                /* If the next char following the keyword or identifier
				   is not a blank, (, ), or a comma, then append this
					 character to temp_string, and flag this as an error */
                temp_string[i++] = *cur++;
                add_to_list(tok_list, temp_string, error, INVALID);
                rc = INVALID;
                done = true;
            } else {
                // We have an identifier with at least 1 character
                // Now check if this ident is a keyword
                for (j = 0, found_keyword = false; j < TOTAL_KEYWORDS_PLUS_TYPE_NAMES; j++) {
                    if ((strcasecmp(keyword_table[j], temp_string) == 0)) {
                        found_keyword = true;
                        break;
                    }
                }

                if (found_keyword) {
                    if (KEYWORD_OFFSET + j < K_CREATE)
                        t_class = type_name;
                    else if (KEYWORD_OFFSET + j >= F_SUM)
                        t_class = function_name;
                    else
                        t_class = keyword;

                    add_to_list(tok_list, temp_string, t_class, KEYWORD_OFFSET + j);
                } else {
                    if (strlen(temp_string) <= MAX_IDENT_LEN)
                        add_to_list(tok_list, temp_string, identifier, IDENT);
                    else {
                        add_to_list(tok_list, temp_string, error, INVALID);
                        rc = INVALID;
                        done = true;
                    }
                }

                if (!*cur) {
                    add_to_list(tok_list, "", terminator, EOC);
                    done = true;
                }
            }
        } else if (isdigit(*cur)) {
            // find valid number
            do {
                temp_string[i++] = *cur++;
            } while (isdigit(*cur));

            if (!(strchr(NUMBER_BREAK, *cur))) {
                /* If the next char following the keyword or identifier
				   is not a blank or a ), then append this
					 character to temp_string, and flag this as an error */
                temp_string[i++] = *cur++;
                add_to_list(tok_list, temp_string, error, INVALID);
                rc = INVALID;
                done = true;
            } else {
                add_to_list(tok_list, temp_string, constant, INT_LITERAL);

                if (!*cur) {
                    add_to_list(tok_list, "", terminator, EOC);
                    done = true;
                }
            }
        } else if ((*cur == '(') || (*cur == ')') || (*cur == ',') || (*cur == '*') || (*cur == '=') || (*cur == '<') || (*cur == '>')) {
            /* Catch all the symbols here. Note: no look ahead here. */
            int t_value;
            switch (*cur) {
                case '(':
                    t_value = S_LEFT_PAREN;
                    break;
                case ')':
                    t_value = S_RIGHT_PAREN;
                    break;
                case ',':
                    t_value = S_COMMA;
                    break;
                case '*':
                    t_value = S_STAR;
                    break;
                case '=':
                    t_value = S_EQUAL;
                    break;
                case '<':
                    t_value = S_LESS;
                    break;
                case '>':
                    t_value = S_GREATER;
                    break;
            }

            temp_string[i++] = *cur++;

            add_to_list(tok_list, temp_string, symbol, t_value);

            if (!*cur) {
                add_to_list(tok_list, "", terminator, EOC);
                done = true;
            }
        } else if (*cur == '\'') {
            /* Find STRING_LITERRAL */
            int t_class;
            cur++;
            do {
                temp_string[i++] = *cur++;
            } while ((*cur) && (*cur != '\''));

            temp_string[i] = '\0';

            if (!*cur) {
                /* If we reach the end of line */
                add_to_list(tok_list, temp_string, error, INVALID);
                rc = INVALID;
                done = true;
            } else /* must be a ' */
            {
                add_to_list(tok_list, temp_string, constant, STRING_LITERAL);
                cur++;
                if (!*cur) {
                    add_to_list(tok_list, "", terminator, EOC);
                    done = true;
                }
            }
        } else {
            if (!*cur) {
                add_to_list(tok_list, "", terminator, EOC);
                done = true;
            } else {
                /* not a ident, number, or valid symbol */
                temp_string[i++] = *cur++;
                add_to_list(tok_list, temp_string, error, INVALID);
                rc = INVALID;
                done = true;
            }
        }
    }

    return rc;
}

void add_to_list(token_list **tok_list, char *tmp, int t_class, int t_value) {
    token_list *cur = *tok_list;
    token_list *ptr = NULL;

    // printf("%16s \t%d \t %d\n",tmp, t_class, t_value);

    ptr = (token_list *)calloc(1, sizeof(token_list));
    strcpy(ptr->tok_string, tmp);
    ptr->tok_class = t_class;
    ptr->tok_value = t_value;
    ptr->next = NULL;

    if (cur == NULL)
        *tok_list = ptr;
    else {
        while (cur->next != NULL)
            cur = cur->next;

        cur->next = ptr;
    }
    return;
}

int do_semantic(token_list *tok_list) {
    int rc = 0, cur_cmd = INVALID_STATEMENT;
    bool unique = false;
    token_list *cur = tok_list;

    if ((cur->tok_value == K_CREATE) &&
        ((cur->next != NULL) && (cur->next->tok_value == K_TABLE))) {
        printf("CREATE TABLE statement\n");
        cur_cmd = CREATE_TABLE;
        cur = cur->next->next;
    } else if ((cur->tok_value == K_DROP) &&
               ((cur->next != NULL) && (cur->next->tok_value == K_TABLE))) {
        printf("DROP TABLE statement\n");
        cur_cmd = DROP_TABLE;
        cur = cur->next->next;
    } else if ((cur->tok_value == K_LIST) &&
               ((cur->next != NULL) && (cur->next->tok_value == K_TABLE))) {
        printf("LIST TABLE statement\n");
        cur_cmd = LIST_TABLE;
        cur = cur->next->next;
    } else if ((cur->tok_value == K_LIST) &&
               ((cur->next != NULL) && (cur->next->tok_value == K_SCHEMA))) {
        printf("LIST SCHEMA statement\n");
        cur_cmd = LIST_SCHEMA;
        cur = cur->next->next;
    } else if ((cur->tok_value == K_INSERT) &&
               ((cur->next != NULL) && (cur->next->tok_value == K_INTO))) {
        cur_cmd = INSERT;
        cur = cur->next->next;
    } else if ((cur->tok_value == K_SELECT) &&
               (cur->next != NULL)) {
        cur_cmd = SELECT;
        cur = cur->next;
    } else if ((cur->tok_value == K_DELETE) &&
               ((cur->next != NULL) && (cur->next->tok_value == K_FROM))) {
        cur_cmd = DELETE;
        cur = cur->next->next;
    } else if (cur->tok_value == K_UPDATE) {
        cur_cmd = UPDATE;
        cur = cur->next;
    } else {
        printf("Invalid statement\n");
        rc = cur_cmd;
    }

    if (cur_cmd != INVALID_STATEMENT) {
        switch (cur_cmd) {
            case CREATE_TABLE:
                rc = sem_create_table(cur);
                break;
            case DROP_TABLE:
                rc = sem_drop_table(cur);
                break;
            case LIST_TABLE:
                rc = sem_list_tables();
                break;
            case LIST_SCHEMA:
                rc = sem_list_schema(cur);
                break;
            case INSERT:
                rc = sem_insert_schema(cur);
                break;
            case SELECT:
                rc = sem_select_schema(cur);
                break;
            case DELETE:
                rc = sem_delete_schema(cur);
                break;
            case UPDATE:
                rc = sem_update_schema(cur);
                break;
            default:; /* no action */
        }
    }

    return rc;
}

int sem_create_table(token_list *t_list) {
    int rc = 0;
    token_list *cur;
    tpd_entry tab_entry;
    tpd_entry *new_entry = NULL;
    bool column_done = false;
    int cur_id = 0;
    cd_entry col_entry[MAX_NUM_COL];

    memset(&tab_entry, '\0', sizeof(tpd_entry));
    cur = t_list;
    if ((cur->tok_class != keyword) &&
        (cur->tok_class != identifier) &&
        (cur->tok_class != type_name)) {
        // Error
        rc = INVALID_TABLE_NAME;
        cur->tok_value = INVALID;
    } else {
        if ((new_entry = get_tpd_from_list(cur->tok_string)) != NULL) {
            rc = DUPLICATE_TABLE_NAME;
            cur->tok_value = INVALID;
            printf("Table: %s already exists and error code: %d", cur->tok_string, rc);
        } else {
            strcpy(tab_entry.table_name, cur->tok_string);
            cur = cur->next;
            if (cur->tok_value != S_LEFT_PAREN) {
                //Error
                rc = INVALID_TABLE_DEFINITION;
                cur->tok_value = INVALID;
                printf("Invalid table definition and error code: %d", rc);
            } else {
                memset(&col_entry, '\0', (MAX_NUM_COL * sizeof(cd_entry)));

                /* Now build a set of column entries */
                cur = cur->next;
                do {
                    if ((cur->tok_class != keyword) &&
                        (cur->tok_class != identifier) &&
                        (cur->tok_class != type_name)) {
                        // Error
                        rc = INVALID_COLUMN_NAME;
                        cur->tok_value = INVALID;
                    } else {
                        int i;
                        for (i = 0; i < cur_id; i++) {
                            /* make column name case sensitive */
                            if (strcmp(col_entry[i].col_name, cur->tok_string) == 0) {
                                rc = DUPLICATE_COLUMN_NAME;
                                cur->tok_value = INVALID;
                                break;
                            }
                        }

                        if (!rc) {
                            strcpy(col_entry[cur_id].col_name, cur->tok_string);
                            col_entry[cur_id].col_id = cur_id;
                            col_entry[cur_id].not_null = false; /* set default */

                            cur = cur->next;
                            if (cur->tok_class != type_name) {
                                // Error
                                rc = INVALID_TYPE_NAME;
                                cur->tok_value = INVALID;
                            } else {
                                /* Set the column type here, int or char */
                                col_entry[cur_id].col_type = cur->tok_value;
                                cur = cur->next;

                                if (col_entry[cur_id].col_type == T_INT) {
                                    if ((cur->tok_value != S_COMMA) &&
                                        (cur->tok_value != K_NOT) &&
                                        (cur->tok_value != S_RIGHT_PAREN)) {
                                        rc = INVALID_COLUMN_DEFINITION;
                                        cur->tok_value = INVALID;
                                    } else {
                                        col_entry[cur_id].col_len = sizeof(int);

                                        if ((cur->tok_value == K_NOT) &&
                                            (cur->next->tok_value != K_NULL)) {
                                            rc = INVALID_COLUMN_DEFINITION;
                                            cur->tok_value = INVALID;
                                        } else if ((cur->tok_value == K_NOT) &&
                                                   (cur->next->tok_value == K_NULL)) {
                                            col_entry[cur_id].not_null = true;
                                            cur = cur->next->next;
                                        }

                                        if (!rc) {
                                            /* I must have either a comma or right paren */
                                            if ((cur->tok_value != S_RIGHT_PAREN) &&
                                                (cur->tok_value != S_COMMA)) {
                                                rc = INVALID_COLUMN_DEFINITION;
                                                cur->tok_value = INVALID;
                                            } else {
                                                if (cur->tok_value == S_RIGHT_PAREN) {
                                                    column_done = true;
                                                }
                                                cur = cur->next;
                                            }
                                        }
                                    }
                                }  // end of T_INT processing
                                else {
                                    // It must be char() or varchar()
                                    if (cur->tok_value != S_LEFT_PAREN) {
                                        rc = INVALID_COLUMN_DEFINITION;
                                        cur->tok_value = INVALID;
                                    } else {
                                        /* Enter char(n) processing */
                                        cur = cur->next;

                                        if (cur->tok_value != INT_LITERAL) {
                                            rc = INVALID_COLUMN_LENGTH;
                                            cur->tok_value = INVALID;
                                        } else {
                                            /* Got a valid integer - convert */
                                            col_entry[cur_id].col_len = atoi(cur->tok_string);
                                            cur = cur->next;

                                            if (cur->tok_value != S_RIGHT_PAREN) {
                                                rc = INVALID_COLUMN_DEFINITION;
                                                cur->tok_value = INVALID;
                                            } else {
                                                cur = cur->next;

                                                if ((cur->tok_value != S_COMMA) &&
                                                    (cur->tok_value != K_NOT) &&
                                                    (cur->tok_value != S_RIGHT_PAREN)) {
                                                    rc = INVALID_COLUMN_DEFINITION;
                                                    cur->tok_value = INVALID;
                                                } else {
                                                    if ((cur->tok_value == K_NOT) &&
                                                        (cur->next->tok_value != K_NULL)) {
                                                        rc = INVALID_COLUMN_DEFINITION;
                                                        cur->tok_value = INVALID;
                                                    } else if ((cur->tok_value == K_NOT) &&
                                                               (cur->next->tok_value == K_NULL)) {
                                                        col_entry[cur_id].not_null = true;
                                                        cur = cur->next->next;
                                                    }

                                                    if (!rc) {
                                                        /* I must have either a comma or right paren */
                                                        if ((cur->tok_value != S_RIGHT_PAREN) && (cur->tok_value != S_COMMA)) {
                                                            rc = INVALID_COLUMN_DEFINITION;
                                                            cur->tok_value = INVALID;
                                                        } else {
                                                            if (cur->tok_value == S_RIGHT_PAREN) {
                                                                column_done = true;
                                                            }
                                                            cur = cur->next;
                                                        }
                                                    }
                                                }
                                            }
                                        } /* end char(n) processing */
                                    }
                                } /* end char processing */
                            }
                        }  // duplicate column name
                    }      // invalid column name

                    /* If rc=0, then get ready for the next column */
                    if (!rc) {
                        cur_id++;
                    }

                } while ((rc == 0) && (!column_done));

                if ((column_done) && (cur->tok_value != EOC)) {
                    rc = INVALID_TABLE_DEFINITION;
                    cur->tok_value = INVALID;
                }

                if (!rc) {
                    /* Now finished building tpd and add it to the tpd list */
                    tab_entry.num_columns = cur_id;
                    tab_entry.tpd_size = sizeof(tpd_entry) +
                                         sizeof(cd_entry) * tab_entry.num_columns;
                    tab_entry.cd_offset = sizeof(tpd_entry);
                    new_entry = (tpd_entry *)calloc(1, tab_entry.tpd_size);

                    if (new_entry == NULL) {
                        rc = MEMORY_ERROR;
                    } else {
                        memcpy((void *)new_entry,
                               (void *)&tab_entry,
                               sizeof(tpd_entry));

                        memcpy((void *)((char *)new_entry + sizeof(tpd_entry)),
                               (void *)col_entry,
                               sizeof(cd_entry) * tab_entry.num_columns);

                        rc = add_tpd_to_list(new_entry);

                        // create table file
                        if (!rc) {
                            rc = create_table_file(tab_entry, col_entry);
                            if (rc) {
                                cur->tok_value = INVALID;
                            }
                        }

                        free(new_entry);
                    }
                }
            }
        }
    }
    return rc;
}

int sem_drop_table(token_list *t_list) {
    int rc = 0;
    token_list *cur;
    tpd_entry *tab_entry = NULL;

    cur = t_list;
    if ((cur->tok_class != keyword) &&
        (cur->tok_class != identifier) &&
        (cur->tok_class != type_name)) {
        // Error
        rc = INVALID_TABLE_NAME;
        cur->tok_value = INVALID;
    } else {
        if (cur->next->tok_value != EOC) {
            rc = INVALID_STATEMENT;
            cur->next->tok_value = INVALID;
        } else {
            if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL) {
                rc = TABLE_NOT_EXIST;
                cur->tok_value = INVALID;
            } else {
                /* Found a valid tpd, drop it from tpd list */
                rc = drop_tpd_from_list(cur->tok_string);
                // delte the tab file
                if (!rc) {
                    char table_filename[MAX_IDENT_LEN + 5];
                    sprintf(table_filename, "%s.tab", cur->tok_string);
                    if (remove(table_filename) != 0) {
                        rc = FILE_DELETE_ERROR;
                        cur->tok_value = INVALID;
                    }
                }
            }
        }
    }

    return rc;
}

int sem_list_tables() {
    int rc = 0;
    int num_tables = g_tpd_list->num_tables;
    tpd_entry *cur = &(g_tpd_list->tpd_start);

    if (num_tables == 0) {
        printf("\nThere are currently no tables defined\n");
    } else {
        printf("\nTable List\n");
        printf("*****************\n");
        while (num_tables-- > 0) {
            printf("%s\n", cur->table_name);
            if (num_tables > 0) {
                cur = (tpd_entry *)((char *)cur + cur->tpd_size);
            }
        }
        printf("****** End ******\n");
    }

    return rc;
}

int sem_list_schema(token_list *t_list) {
    int rc = 0;
    token_list *cur;
    tpd_entry *tab_entry = NULL;
    cd_entry *col_entry = NULL;
    char tab_name[MAX_IDENT_LEN + 1];
    char filename[MAX_IDENT_LEN + 1];
    bool report = false;
    FILE *fhandle = NULL;
    int i = 0;

    cur = t_list;

    if (cur->tok_value != K_FOR) {
        rc = INVALID_STATEMENT;
        cur->tok_value = INVALID;
    } else {
        cur = cur->next;

        if ((cur->tok_class != keyword) &&
            (cur->tok_class != identifier) &&
            (cur->tok_class != type_name)) {
            // Error
            rc = INVALID_TABLE_NAME;
            cur->tok_value = INVALID;
        } else {
            memset(filename, '\0', MAX_IDENT_LEN + 1);
            strcpy(tab_name, cur->tok_string);
            cur = cur->next;

            if (cur->tok_value != EOC) {
                if (cur->tok_value == K_TO) {
                    cur = cur->next;

                    if ((cur->tok_class != keyword) &&
                        (cur->tok_class != identifier) &&
                        (cur->tok_class != type_name)) {
                        // Error
                        rc = INVALID_REPORT_FILE_NAME;
                        cur->tok_value = INVALID;
                    } else {
                        if (cur->next->tok_value != EOC) {
                            rc = INVALID_STATEMENT;
                            cur->next->tok_value = INVALID;
                        } else {
                            /* We have a valid file name */
                            strcpy(filename, cur->tok_string);
                            report = true;
                        }
                    }
                } else {
                    /* Missing the TO keyword */
                    rc = INVALID_STATEMENT;
                    cur->tok_value = INVALID;
                }
            }

            if (!rc) {
                if ((tab_entry = get_tpd_from_list(tab_name)) == NULL) {
                    rc = TABLE_NOT_EXIST;
                    cur->tok_value = INVALID;
                } else {
                    if (report) {
                        if ((fhandle = fopen(filename, "a+tc")) == NULL) {
                            rc = FILE_OPEN_ERROR;
                        }
                    }

                    if (!rc) {
                        /* Find correct tpd, need to parse column and index information */

                        /* First, write the tpd_entry information */
                        printf("Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
                        printf("Table Name               (table_name)  = %s\n", tab_entry->table_name);
                        printf("Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
                        printf("Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
                        printf("Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags);

                        if (report) {
                            fprintf(fhandle, "Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
                            fprintf(fhandle, "Table Name               (table_name)  = %s\n", tab_entry->table_name);
                            fprintf(fhandle, "Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
                            fprintf(fhandle, "Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
                            fprintf(fhandle, "Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags);
                        }

                        /* Next, write the cd_entry information */
                        for (i = 0, col_entry = (cd_entry *)((char *)tab_entry + tab_entry->cd_offset);
                             i < tab_entry->num_columns; i++, col_entry++) {
                            printf("Column Name   (col_name) = %s\n", col_entry->col_name);
                            printf("Column Id     (col_id)   = %d\n", col_entry->col_id);
                            printf("Column Type   (col_type) = %d\n", col_entry->col_type);
                            printf("Column Length (col_len)  = %d\n", col_entry->col_len);
                            printf("Not Null flag (not_null) = %d\n\n", col_entry->not_null);

                            if (report) {
                                fprintf(fhandle, "Column Name   (col_name) = %s\n", col_entry->col_name);
                                fprintf(fhandle, "Column Id     (col_id)   = %d\n", col_entry->col_id);
                                fprintf(fhandle, "Column Type   (col_type) = %d\n", col_entry->col_type);
                                fprintf(fhandle, "Column Length (col_len)  = %d\n", col_entry->col_len);
                                fprintf(fhandle, "Not Null Flag (not_null) = %d\n\n", col_entry->not_null);
                            }
                        }

                        if (report) {
                            fflush(fhandle);
                            fclose(fhandle);
                        }
                    }  // File open error
                }      // Table not exist
            }          // no semantic errors
        }              // Invalid table name
    }                  // Invalid statement

    return rc;
}

int initialize_tpd_list() {
    int rc = 0;
    FILE *fhandle = NULL;
    //	struct _stat file_stat;
    struct stat file_stat;

    /* Open for read */
    if ((fhandle = fopen("dbfile.bin", "rbc")) == NULL) {
        if ((fhandle = fopen("dbfile.bin", "wbc")) == NULL) {
            rc = FILE_OPEN_ERROR;
        } else {
            g_tpd_list = NULL;
            g_tpd_list = (tpd_list *)calloc(1, sizeof(tpd_list));

            if (!g_tpd_list) {
                rc = MEMORY_ERROR;
            } else {
                g_tpd_list->list_size = sizeof(tpd_list);
                fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
                fflush(fhandle);
                fclose(fhandle);
            }
        }
    } else {
        /* There is a valid dbfile.bin file - get file size */
        //		_fstat(_fileno(fhandle), &file_stat);
        fstat(fileno(fhandle), &file_stat);
        // printf("dbfile.bin size = %d\n", file_stat.st_size);

        g_tpd_list = (tpd_list *)calloc(1, file_stat.st_size);

        if (!g_tpd_list) {
            rc = MEMORY_ERROR;
        } else {
            fread(g_tpd_list, file_stat.st_size, 1, fhandle);
            fflush(fhandle);
            fclose(fhandle);

            if (g_tpd_list->list_size != file_stat.st_size) {
                rc = DBFILE_CORRUPTION;
            }
        }
    }

    return rc;
}

int add_tpd_to_list(tpd_entry *tpd) {
    int rc = 0;
    int old_size = 0;
    FILE *fhandle = NULL;

    if ((fhandle = fopen("dbfile.bin", "wbc")) == NULL) {
        rc = FILE_OPEN_ERROR;
    } else {
        old_size = g_tpd_list->list_size;

        if (g_tpd_list->num_tables == 0) {
            /* If this is an empty list, overlap the dummy header */
            g_tpd_list->num_tables++;
            g_tpd_list->list_size += (tpd->tpd_size - sizeof(tpd_entry));
            fwrite(g_tpd_list, old_size - sizeof(tpd_entry), 1, fhandle);
        } else {
            /* There is at least 1, just append at the end */
            g_tpd_list->num_tables++;
            g_tpd_list->list_size += tpd->tpd_size;
            fwrite(g_tpd_list, old_size, 1, fhandle);
        }

        fwrite(tpd, tpd->tpd_size, 1, fhandle);
        fflush(fhandle);
        fclose(fhandle);
    }

    return rc;
}

int drop_tpd_from_list(char *tabname) {
    int rc = 0;
    tpd_entry *cur = &(g_tpd_list->tpd_start);
    int num_tables = g_tpd_list->num_tables;
    bool found = false;
    int count = 0;

    if (num_tables > 0) {
        while ((!found) && (num_tables-- > 0)) {
            if (strcasecmp(cur->table_name, tabname) == 0) {
                /* found it */
                found = true;
                int old_size = 0;
                FILE *fhandle = NULL;

                if ((fhandle = fopen("dbfile.bin", "wbc")) == NULL) {
                    rc = FILE_OPEN_ERROR;
                } else {
                    old_size = g_tpd_list->list_size;

                    if (count == 0) {
                        /* If this is the first entry */
                        g_tpd_list->num_tables--;

                        if (g_tpd_list->num_tables == 0) {
                            /* This is the last table, null out dummy header */
                            memset((void *)g_tpd_list, '\0', sizeof(tpd_list));
                            g_tpd_list->list_size = sizeof(tpd_list);
                            fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
                        } else {
                            /* First in list, but not the last one */
                            g_tpd_list->list_size -= cur->tpd_size;

                            /* First, write the 8 byte header */
                            fwrite(g_tpd_list, sizeof(tpd_list) - sizeof(tpd_entry),
                                   1, fhandle);

                            /* Now write everything starting after the cur entry */
                            fwrite((char *)cur + cur->tpd_size,
                                   old_size - cur->tpd_size -
                                       (sizeof(tpd_list) - sizeof(tpd_entry)),
                                   1, fhandle);
                        }
                    } else {
                        /* This is NOT the first entry - count > 0 */
                        g_tpd_list->num_tables--;
                        g_tpd_list->list_size -= cur->tpd_size;

                        /* First, write everything from beginning to cur */
                        fwrite(g_tpd_list, ((char *)cur - (char *)g_tpd_list),
                               1, fhandle);

                        /* Check if cur is the last entry. Note that g_tdp_list->list_size
						   has already subtracted the cur->tpd_size, therefore it will
						   point to the start of cur if cur was the last entry */
                        if ((char *)g_tpd_list + g_tpd_list->list_size == (char *)cur) {
                            /* If true, nothing else to write */
                        } else {
                            /* NOT the last entry, copy everything from the beginning of the
							   next entry which is (cur + cur->tpd_size) and the remaining size */
                            fwrite((char *)cur + cur->tpd_size,
                                   old_size - cur->tpd_size -
                                       ((char *)cur - (char *)g_tpd_list),
                                   1, fhandle);
                        }
                    }

                    fflush(fhandle);
                    fclose(fhandle);
                }
            } else {
                if (num_tables > 0) {
                    cur = (tpd_entry *)((char *)cur + cur->tpd_size);
                    count++;
                }
            }
        }
    }

    if (!found) {
        rc = INVALID_TABLE_NAME;
    }

    return rc;
}

int create_table_file(tpd_entry tab_entry, cd_entry cd_entries[]) {
    int rc = 0;
    table_file_header tab_header;

    int record_size = 0;
    for (int i = 0; i < tab_entry.num_columns; i++) {
        record_size += (1 + cd_entries[i].col_len);
    }
    // size should be multiplier of 4
    int r = record_size % 4;
    record_size = r ? (record_size + 4 - r) : record_size;

    // table details
    tab_header.file_size = sizeof(table_file_header);
    tab_header.record_size = record_size;
    tab_header.num_records = 0;
    tab_header.record_offset = sizeof(table_file_header);
    tab_header.file_header_flag = 0;
    tab_header.tpd_ptr = NULL;

    char table_filename[MAX_IDENT_LEN + 5];
    sprintf(table_filename, "%s.tab", tab_entry.table_name);

    // create file
    FILE *fhandle = NULL;
    if ((fhandle = fopen(table_filename, "wbc")) == NULL) {
        rc = FILE_OPEN_ERROR;
    } else {
        fwrite((void *)&tab_header, tab_header.file_size, 1, fhandle);
        fflush(fhandle);
        fclose(fhandle);
    }

    if (debug) {
        printf("%s is created successfully", table_filename);
    }

    return rc;
}

int sem_insert_schema(token_list *t_list) {
    int rc = 0;
    token_list *cur = t_list;

    //  Check for table name
    if ((cur->tok_class != keyword) &&
        (cur->tok_class != identifier) &&
        (cur->tok_class != type_name)) {
        // Error
        rc = INVALID_TABLE_NAME;
        cur->tok_value = INVALID;
        return rc;
    }

    // check table in tpd list
    tpd_entry *tab_entry = get_tpd_from_list(cur->tok_string);
    if (tab_entry == NULL) {
        rc = TABLE_NOT_EXIST;
        printf("Table: %s does not exist\n", cur->tok_string);
        return rc;
    }

    // check values
    cur = cur->next;
    if ((cur->tok_class != keyword) &&
        (cur->tok_value != K_VALUES)) {
        // Error
        rc = INVALID_STATEMENT;
        cur->tok_value = INVALID;
        printf("VALUES should be followed after table name\n");
        return rc;
    }

    // left paranthesis
    cur = cur->next;
    if ((cur->tok_class != symbol) &&
        (cur->tok_value != S_LEFT_PAREN)) {
        // Error
        rc = INVALID_STATEMENT;
        cur->tok_value = INVALID;
        printf(" Column values should between '(' and ')'\n");
        return rc;
    }

    cur = cur->next;
    int i = 0;
    // columns placeholder
    col_item col_items[tab_entry->num_columns];
    for (i = 0; i < tab_entry->num_columns; i++) {
        col_items[i].is_null = false;

        switch (cur->tok_value) {
            case K_NULL:
                col_items[i].is_null = true;
                break;
            case INT_LITERAL:
                col_items[i].int_val = atoi(cur->tok_string);
                break;
            case STRING_LITERAL:
                strcpy(col_items[i].string_val, cur->tok_string);
                break;
            default: {
                rc = INVALID;
                break;
            }
        }
        col_items[i].token = cur;
        cur = cur->next;
        if (cur->tok_value == S_COMMA) {
            // nothing to do
        } else if (cur->tok_value == S_RIGHT_PAREN) {
            break;
        } else {
            rc = INVALID_STATEMENT;
            break;
        }
        cur = cur->next;
    }
    if (rc || (i + 1) != tab_entry->num_columns) {
        cur->tok_value = INVALID;
        printf("Less column values are present in the statement\n");
        return rc;
    }

    // EOC
    cur = cur->next;
    if (cur->tok_value != EOC) {
        rc = INVALID_STATEMENT;
        cur->tok_value = INVALID;
        return rc;
    }

    cd_entry *cd_entries = NULL;
    get_cd_entries(tab_entry, &cd_entries);

    // validate column values
    rc = validate_columns(tab_entry->num_columns, col_items, cd_entries);

    if (rc) {
        return rc;
    }

    // Load table file
    table_file_header *table_header = NULL;
    if ((rc = load_table_file(tab_entry, &table_header)) != 0) {
        return rc;
    }

    char *record_bytes = (char *)malloc(table_header->record_size);
    col_item *col_items_ptr[tab_entry->num_columns];
    for (int i = 0; i < tab_entry->num_columns; i++) {
        col_items_ptr[i] = &col_items[i];
    }

    copy_columns_as_bytes(cd_entries, col_items_ptr, tab_entry->num_columns, record_bytes, table_header->record_size);

    // Append the new record to the .tab file.
    char table_filename[MAX_IDENT_LEN + 5];
    sprintf(table_filename, "%s.tab", table_header->tpd_ptr->table_name);
    FILE *fhandle = NULL;
    if ((fhandle = fopen(table_filename, "wbc")) == NULL) {
        rc = FILE_OPEN_ERROR;
    } else {
        // Add one more record in table header.
        int old_table_file_size = table_header->file_size;
        table_header->num_records++;
        table_header->file_size += table_header->record_size;
        table_header->tpd_ptr = NULL;  // Reset tpd pointer.

        fwrite(table_header, old_table_file_size, 1, fhandle);
        fwrite(record_bytes, table_header->record_size, 1, fhandle);
        fflush(fhandle);
        fclose(fhandle);
    }
    free(record_bytes);
    free(table_header);

    printf("\n 1 record inserted successfully");
    return rc;
}

int sem_select_schema(token_list *t_list) {
    int rc = 0;
    token_list *cur = t_list;
    int wildcard_col_idx = -1;

    // list of columns in the selector
    col_info col_infos[MAX_NUM_COL];
    //parse column aggregates
    int aggregate_type = 0;
    int aggregate_col_id = -1;
    int col_counter = 0;
    bool all_cols_parsed = false;

    // // only one aggregate funtion is acceptable in the select statement.
    // // Ex: select count(name) from class
    // // No aggregate then collect the order of columns need to be returned/printed out.
    // // AVG, SUM, COUNT
    // if (cur->tok_class == function_name &&
    //     cur->next->tok_value == S_LEFT_PAREN) {
    //     aggregate_type = cur->tok_value;
    //     //  check for column name
    //     if ((cur->tok_class == keyword) ||
    //         (cur->tok_class == identifier) ||
    //         (cur->tok_class == type_name)) {
    //         rc = INVALID_COLUMN_NAME;
    //         cur->tok_value = INVALID;
    //         return rc;
    //     }
    //     cur = cur->next->next;
    //     // add columns in parsed order
    //     strcpy(col_infos[col_counter].name, cur->tok_string);
    //     col_infos[col_counter].token = cur;
    //     if (strcmp(cur->tok_string, "*") == 0) {
    //         wildcard_col_idx = 0;
    //     }

    //     cur = cur->next;
    //     if (cur->tok_value != S_RIGHT_PAREN) {
    //         // Error column name.
    //         rc = INVALID_COLUMN_NAME;
    //         cur->tok_value = INVALID;
    //         return rc;
    //     } else {
    //         cur = cur->next;
    //         col_counter++;
    //         all_cols_parsed = true;
    //     }
    // }

    // now parse if no aggregator function is passed in the statement
    while (!all_cols_parsed) {
        //  check for column name
        // if * is already read in previous column position then return error
        // More than 1 '*' cannot be present in the statement.
        // Ec: SELECT *, name, * from class => INVALID
        if (((cur->tok_class == keyword) &&
             (cur->tok_class == identifier) &&
             (cur->tok_class == type_name)) ||
            ((cur->tok_value == S_STAR) && (wildcard_col_idx == 0))) {
            rc = INVALID_COLUMN_NAME;
            cur->tok_value = INVALID;
            return rc;
        }

        if (strcmp(cur->tok_string, "*") == 0) {
            // Wildcard must appear only once, as the first column name.
            if (wildcard_col_idx == -1 && col_counter == 0) {
                wildcard_col_idx = 0;
            } else {
                rc = INVALID_COLUMN_NAME;
                cur->tok_value = INVALID;
                return rc;
            }
        }

        // only one aggregate funtion is acceptable in the select statement.
        // Ex: select count(name) from class
        // No aggregate then collect the order of columns need to be returned/printed out.
        // AVG, SUM, COUNT
        if (cur->tok_class == function_name &&
            cur->next->tok_value == S_LEFT_PAREN) {
            aggregate_type = cur->tok_value;
            //  check for column name
            if ((cur->tok_class == keyword) ||
                (cur->tok_class == identifier) ||
                (cur->tok_class == type_name)) {
                rc = INVALID_COLUMN_NAME;
                cur->tok_value = INVALID;
                return rc;
            }
            cur = cur->next->next;
            // add columns in parsed order
            strcpy(col_infos[col_counter].name, cur->tok_string);
            col_infos[col_counter].token = cur;
            if (strcmp(cur->tok_string, "*") == 0) {
                wildcard_col_idx = 0;
            }

            // aggregate column id
            aggregate_col_id = col_counter;

            cur = cur->next;
            if (cur->tok_value != S_RIGHT_PAREN) {
                // Error column name.
                rc = INVALID_COLUMN_NAME;
                cur->tok_value = INVALID;
                return rc;
            } else {
                cur = cur->next;
                col_counter++;
                // all_cols_parsed = true;
            }
            if (cur->tok_value == S_COMMA) {
                cur = cur->next;
            } else {
                all_cols_parsed = true;
            }
            continue;
        }
        // add column
        strcpy(col_infos[col_counter].name, cur->tok_string);
        col_infos[col_counter].token = cur;
        col_counter++;
        cur = cur->next;
        if (cur->tok_value == S_COMMA) {
            cur = cur->next;
        } else {
            all_cols_parsed = true;
        }
    }

    // check for the FROM keyword
    // cur = cur->next;
    if ((cur->tok_class != keyword) &&
        (cur->tok_value != K_FROM)) {
        // Error
        rc = INVALID_STATEMENT;
        cur->tok_value = INVALID;
        return rc;
    }
    cur = cur->next;

    // get table name
    if ((cur->tok_class != keyword) &&
        (cur->tok_class != identifier) &&
        (cur->tok_class != type_name)) {
        // Error
        rc = INVALID_TABLE_NAME;
        cur->tok_value = INVALID;
        return rc;
    }

    // Check whether the table name exists.
    tpd_entry *tab_entry = get_tpd_from_list(cur->tok_string);
    if (tab_entry == NULL) {
        rc = TABLE_NOT_EXIST;
        cur->tok_value = INVALID;
        return rc;
    }

    cd_entry *cd_entries = NULL;
    get_cd_entries(tab_entry, &cd_entries);

    // In case of * add all columns from column-descriptor
    if (wildcard_col_idx == 0) {
        token_list *wildcard_token = col_infos[wildcard_col_idx].token;
        for (int i = 0; i < tab_entry->num_columns; i++) {
            strcpy(col_infos[i].name, cd_entries[i].col_name);
            col_infos[i].token = wildcard_token;
        }
        // update the columns idx to all columns count
        col_counter = tab_entry->num_columns;
    }

    // order the cd_entries as per select statement
    cd_entry *listed_cd_entries[col_counter];
    for (int i = 0; i < col_counter; i++) {
        int col_index = get_cd_entry_index(cd_entries, tab_entry->num_columns,
                                           col_infos[i].name);

        // column name not found
        if (col_index < 0) {
            rc = INVALID_COLUMN_NAME;
            col_infos[i].token->tok_value = INVALID;
            return rc;
        }
        cd_entry tmp_c = cd_entries[col_index];
        if ((i == aggregate_col_id) && ((aggregate_type == F_SUM) || (aggregate_type == F_AVG))) {
            if (cd_entries[col_index].col_type == T_INT) {
                listed_cd_entries[i] = &cd_entries[col_index];
            } else {
                rc = INVALID_STATEMENT;
                col_infos[i].token->tok_value = INVALID;
                return rc;
            }
        } else {
            listed_cd_entries[i] = &cd_entries[col_index];
        }
    }

    // TODO WHERE CLAUSE
    bool has_where_clause = false;
    row_predicate row_filter;
    memset(&row_filter, '\0', sizeof(row_predicate));
    row_filter.type = K_AND;

    cur = cur->next;
    rc = parse_where_clauses(cur, has_where_clause, cd_entries, tab_entry, row_filter);
    if (rc) {
        return rc;
    }

    // TOOD Group By clause
    bool has_group_by_clause = false;
    int group_by_column_id = -1;
    if (cur->tok_value == K_GROUP && cur->next->tok_value == K_BY) {
        cur = cur->next->next;
        // check for column class
        if (cur->tok_class != identifier) {
            rc = INVALID_COLUMN_NAME;
            cur->tok_value = INVALID;
            return rc;
        }
        group_by_column_id = get_cd_entry_index(
            cd_entries, tab_entry->num_columns, cur->tok_string);

        if (group_by_column_id == -1) {
            rc = INVALID_COLUMN_NAME;
            cur->tok_value = INVALID;
            return rc;
        }
        has_group_by_clause = true;
        cur = cur->next;
    }

    // TODO ORDER BY CLAUSE
    bool has_order_by_clause = false;
    bool order_by_desc = false;
    int order_by_column_id = -1;

    if (cur->tok_value == K_ORDER && cur->next->tok_value == K_BY) {
        cur = cur->next->next;
        // check for column class
        if (cur->tok_class != identifier) {
            rc = INVALID_COLUMN_NAME;
            cur->tok_value = INVALID;
            return rc;
        }
        order_by_column_id = get_cd_entry_index(
            cd_entries, tab_entry->num_columns, cur->tok_string);

        if (order_by_column_id == -1) {
            rc = INVALID_COLUMN_NAME;
            cur->tok_value = INVALID;
            return rc;
        }

        // parse for order by
        has_order_by_clause = true;
        cur = cur->next;
        if (cur->tok_value == K_DESC) {
            order_by_desc = true;
            cur = cur->next;
        }
    }

    // check for End of statement token
    if (cur->tok_value != EOC) {
        rc = INVALID_STATEMENT;
        cur->tok_value = INVALID;
        return rc;
    }

    if (has_group_by_clause && aggregate_type == 0) {
        rc = INVALID_STATEMENT;
        printf("\nGroup By should have one aggregate function in statement");
        return rc;
    }

    // load table_file
    table_file_header *tab_header = NULL;
    if ((rc = load_table_file(tab_entry, &tab_header)) != 0) {
        return rc;
    }

    // records ptr
    char *records = NULL;
    if (tab_header->file_size > tab_header->record_offset) {
        records = ((char *)tab_header) + tab_header->record_offset;
    }

    int aggregate_int_sum = 0;
    int num_loaded_records = 0;
    int aggregate_records_count = 0;

    // if (has_group_by_clause) {
    //     rc = parse_and_print_with_group_by(tab_header);
    // }

    // load and filter the rows from table
    int n_rows = tab_header->num_records;
    row_item row_items[n_rows];
    row_item *curr_row = NULL;
    // for (int i = 0; i < n_rows; i++) {
    //     curr_row = &row_items[num_loaded_records];
    //     update_row_item(cd_entries, tab_entry->num_columns, curr_row, records);

    //     // TODO filtering
    //     if (has_where_clause && !is_row_filtered(cd_entries, tab_entry->num_columns, curr_row,
    //                                              &row_filter)) {
    //         // free memory for filtered record
    //         for (int j = 0; j < curr_row->num_fields; j++) {
    //             free(curr_row->value_ptrs[j]);
    //         }
    //         memset(curr_row, '\0', sizeof(row_item));
    //     } else {
    //         num_loaded_records++;

    //         if ((aggregate_type == F_SUM) || (aggregate_type == F_AVG)) {
    //             // SUM(col) or AVG(col), ignore NULL rows.
    //             if (!curr_row->value_ptrs[listed_cd_entries[0]->col_id]->is_null) {
    //                 aggregate_int_sum +=
    //                     curr_row->value_ptrs[listed_cd_entries[0]->col_id]
    //                         ->int_val;
    //                 aggregate_records_count++;
    //             }
    //         } else if (aggregate_type == F_COUNT) {
    //             if (col_counter == 1) {
    //                 // count(col), ignore NULL rows.
    //                 if (!curr_row->value_ptrs[listed_cd_entries[0]->col_id]
    //                          ->is_null) {
    //                     aggregate_records_count++;
    //                 }
    //             } else {
    //                 // count(*), include NULL rows.
    //                 aggregate_records_count++;
    //             }
    //         }
    //     }

    //     // Move forward to next record.
    //     records += tab_header->record_size;
    // }

    std::map<std::string, int> group_by_aggregate_map;
    std::map<std::string, int> group_by_aggregate_records_map;
    for (int i = 0; i < n_rows; i++) {
        curr_row = &row_items[num_loaded_records];
        update_row_item(cd_entries, tab_entry->num_columns, curr_row, records);
        // TODO filtering
        if (has_where_clause && !is_row_filtered(cd_entries, tab_entry->num_columns, curr_row,
                                                 &row_filter)) {
            // free memory for filtered record
            for (int j = 0; j < curr_row->num_fields; j++) {
                free(curr_row->value_ptrs[j]);
            }
            memset(curr_row, '\0', sizeof(row_item));
            // Move forward to next record.
            records += tab_header->record_size;
            continue;
        }
        num_loaded_records++;
        int tmp_aggregate_int_sum = 0;
        int tmp_aggregate_records_count = 0;

        if ((aggregate_type == F_SUM) || (aggregate_type == F_AVG)) {
            // SUM(col) or AVG(col), ignore NULL rows.
            if (!curr_row->value_ptrs[listed_cd_entries[aggregate_col_id]->col_id]->is_null) {
                tmp_aggregate_int_sum +=
                    curr_row->value_ptrs[listed_cd_entries[aggregate_col_id]->col_id]
                        ->int_val;
                tmp_aggregate_records_count++;
            }
        } else if (aggregate_type == F_COUNT) {
            if (col_counter == 1) {
                // count(col), ignore NULL rows.
                if (!curr_row->value_ptrs[listed_cd_entries[aggregate_col_id]->col_id]
                         ->is_null) {
                    tmp_aggregate_records_count++;
                }
            } else {
                // count(*), include NULL rows.
                tmp_aggregate_records_count++;
            }
        }

        if (has_group_by_clause) {
            std::string tmp_col_key;
            if (curr_row->value_ptrs[group_by_column_id]->is_null) {
                tmp_col_key = "NULL";
            } else if (curr_row->value_ptrs[group_by_column_id]->type == T_INT) {
                // std::string s = std::to_string(curr_row->value_ptrs[group_by_column_id]->int_val);
                // tmp_col_key = (char *)s.c_str();
                tmp_col_key = std::to_string(curr_row->value_ptrs[group_by_column_id]->int_val);
            } else {
                tmp_col_key = std::string(curr_row->value_ptrs[group_by_column_id]->string_val);
            }

            auto group_item = group_by_aggregate_map.find(tmp_col_key);
            if (group_item == group_by_aggregate_map.end()) {
                group_by_aggregate_map.insert(std::pair<std::string, int>(tmp_col_key, tmp_aggregate_int_sum));
            } else {
                group_item->second += tmp_aggregate_int_sum;
            }

            auto group_record_item = group_by_aggregate_records_map.find(tmp_col_key);
            if (group_record_item == group_by_aggregate_records_map.end()) {
                group_by_aggregate_records_map.insert(std::pair<std::string, int>(tmp_col_key, tmp_aggregate_records_count));
            } else {
                group_record_item->second += tmp_aggregate_records_count;
            }

        } else {
            aggregate_int_sum += tmp_aggregate_int_sum;
            aggregate_records_count += tmp_aggregate_records_count;
        }

        // Move forward to next record.
        records += tab_header->record_size;
    }

    if (has_group_by_clause) {
        print_table_border(listed_cd_entries, col_counter);
        // print column names
        int col_gap = 0;
        // group_by column name
        int group_col_idx = wildcard_col_idx != -1 ? group_by_column_id : 0;
        printf("%c %s", '|', listed_cd_entries[group_col_idx]->col_name);
        col_gap = column_display_width(listed_cd_entries[group_col_idx]) -
                  strlen(listed_cd_entries[group_col_idx]->col_name) + 1;
        repeat_print_char(' ', col_gap);

        // aggregate column name
        char display_title[MAX_STRING_LEN + 1];
        memset(display_title, '\0', sizeof(display_title));
        if (col_counter == 2) {
            // Aggregate on one column.
            if (aggregate_type == F_SUM) {
                sprintf(display_title, "SUM(%s)", listed_cd_entries[1]->col_name);
            } else if (aggregate_type == F_AVG) {
                sprintf(display_title, "AVG(%s)", listed_cd_entries[1]->col_name);
            } else {  // F_COUNT
                sprintf(display_title, "COUNT(%s)", listed_cd_entries[1]->col_name);
            }
        } else {
            // Aggregate on one column.
            sprintf(display_title, "COUNT(*)");
        }
        int display_width = (strlen(display_title) > strlen(display_title))
                                ? strlen(display_title)
                                : strlen(display_title);

        printf("%c %s", '|', display_title);
        repeat_print_char(' ', display_width - strlen(display_title));
        printf("%c\n", '|');

        print_table_border(listed_cd_entries, col_counter);

        if (has_order_by_clause && order_by_desc) {
            // print results
            std::map<std::string, int>::reverse_iterator aggregate_it = group_by_aggregate_map.rbegin();
            std::map<std::string, int>::reverse_iterator aggregate_records_it = group_by_aggregate_records_map.rbegin();
            while (aggregate_it != group_by_aggregate_map.rend()) {
                printf("| %s ", aggregate_it->first.c_str());
                repeat_print_char(' ', column_display_width(listed_cd_entries[group_col_idx]) - aggregate_it->first.size());

                char display_value[MAX_STRING_LEN + 1];
                memset(display_value, '\0', sizeof(display_value));
                if (aggregate_type == F_SUM) {
                    sprintf(display_value, "%d", aggregate_it->second);
                } else if (aggregate_type == F_AVG) {
                    if (aggregate_records_it->second == 0) {
                        sprintf(display_value, "NaN");
                    } else {
                        sprintf(display_value, "%d", aggregate_it->second / aggregate_records_it->second);
                    }
                } else if (aggregate_type == F_COUNT) {
                    sprintf(display_value, "%d", aggregate_records_it->second);
                }

                printf("| %s ", display_value);
                repeat_print_char(' ', display_width - strlen(display_value));
                printf("|\n");

                aggregate_it++;
                aggregate_records_it++;
            }
        } else {
            // print results
            std::map<std::string, int>::iterator aggregate_it = group_by_aggregate_map.begin();
            std::map<std::string, int>::iterator aggregate_records_it = group_by_aggregate_records_map.begin();
            while (aggregate_it != group_by_aggregate_map.end()) {
                printf("| %s ", aggregate_it->first.c_str());
                repeat_print_char(' ', column_display_width(listed_cd_entries[group_col_idx]) - aggregate_it->first.size());

                char display_value[MAX_STRING_LEN + 1];
                memset(display_value, '\0', sizeof(display_value));
                if (aggregate_type == F_SUM) {
                    sprintf(display_value, "%d", aggregate_it->second);
                } else if (aggregate_type == F_AVG) {
                    if (aggregate_records_it->second == 0) {
                        sprintf(display_value, "NaN");
                    } else {
                        sprintf(display_value, "%d", aggregate_it->second / aggregate_records_it->second);
                    }
                } else if (aggregate_type == F_COUNT) {
                    sprintf(display_value, "%d", aggregate_records_it->second);
                }

                printf("| %s ", display_value);
                repeat_print_char(' ', display_width - strlen(display_value));
                printf("|\n");

                aggregate_it++;
                aggregate_records_it++;
            }
        }

        print_table_border(listed_cd_entries, col_counter);
    }

    else if (aggregate_type == 0) {
        print_table_border(listed_cd_entries, col_counter);
        print_table_column_names(listed_cd_entries, col_infos, col_counter);
        print_table_border(listed_cd_entries, col_counter);

        // sort records
        if (has_order_by_clause) {
            sort_records(row_items, num_loaded_records,
                         &cd_entries[order_by_column_id], order_by_desc);
        }

        for (int i = 0; i < num_loaded_records; i++) {
            print_record_row(listed_cd_entries, col_counter, &row_items[i]);
        }
        print_table_border(listed_cd_entries, col_counter);
    } else {
        print_aggregate_result(aggregate_type, col_counter, aggregate_records_count,
                               aggregate_int_sum, listed_cd_entries);
    }

    free(tab_header);
    for (int i = 0; i < num_loaded_records; i++) {
        for (int j = 0; j < row_items[i].num_fields; j++) {
            free(row_items[i].value_ptrs[j]);
        }
    }
    return rc;
}

int sem_delete_schema(token_list *t_list) {
    int rc = 0;
    token_list *cur = t_list;

    // check for table name
    if ((cur->tok_class != keyword) &&
        (cur->tok_class != identifier) &&
        (cur->tok_class != type_name)) {
        // Error
        rc = INVALID_TABLE_NAME;
        cur->tok_value = INVALID;
        return rc;
    }

    tpd_entry *tab_entry = get_tpd_from_list(cur->tok_string);
    if (tab_entry == NULL) {
        rc = TABLE_NOT_EXIST;
        cur->tok_value = INVALID;
        return rc;
    }

    cd_entry *cd_entries = NULL;
    get_cd_entries(tab_entry, &cd_entries);

    bool has_where_clause = false;
    row_predicate row_filter;
    memset(&row_filter, '\0', sizeof(row_predicate));
    row_filter.type = K_AND;

    cur = cur->next;
    rc = parse_where_clauses(cur, has_where_clause, cd_entries, tab_entry, row_filter);
    if (rc) {
        return rc;
    }

    if (cur->tok_value != EOC) {
        rc = INVALID_STATEMENT;
        cur->tok_value = INVALID;
        return rc;
    }

    table_file_header *tab_header = NULL;
    if ((rc = load_table_file(tab_entry, &tab_header)) != 0) {
        return rc;
    }

    // records
    char *records = NULL;
    if (tab_header->file_size > tab_header->record_offset) {
        records = ((char *)tab_header) + tab_header->record_offset;
    }

    row_item *p_first_row = NULL;
    row_item *p_current_row = NULL;
    row_item *p_previous_row = NULL;
    int num_loaded_records = 0;
    int num_affected_records = 0;

    for (int i = 0; i < tab_header->num_records; i++) {
        if (p_previous_row == NULL) {
            p_current_row = (row_item *)malloc(sizeof(row_item));
            p_first_row = p_current_row;
        } else {
            p_current_row = (row_item *)malloc(sizeof(row_item));
            p_previous_row->next = p_current_row;
        }
        update_row_item(cd_entries, tab_entry->num_columns, p_current_row,
                        records);
        num_loaded_records++;

        // filter
        if ((!has_where_clause) ||
            is_row_filtered(cd_entries, tab_entry->num_columns, p_current_row,
                            &row_filter)) {
            free_row_item(p_current_row, false);
            p_current_row = NULL;
            if (p_previous_row == NULL) {
                p_first_row = NULL;
            } else {
                p_previous_row->next = NULL;
            }
            num_affected_records++;
        } else {
            p_previous_row = p_current_row;
        }

        records += tab_header->record_size;
    }

    if (num_affected_records > 0) {
        // Write records back to .tab file.
        rc = save_records_to_file(tab_header, p_first_row);
    }

    free_row_item(p_first_row, true);
    free(tab_header);
    if (!rc) {
        printf("%d rows are deleted", num_affected_records);
    }

    return rc;
}

int sem_update_schema(token_list *t_list) {
    int rc = 0;
    token_list *cur = t_list;

    // check for table name
    if ((cur->tok_class != keyword) &&
        (cur->tok_class != identifier) &&
        (cur->tok_class != type_name)) {
        // Error
        rc = INVALID_TABLE_NAME;
        cur->tok_value = INVALID;
        return rc;
    }

    tpd_entry *tab_entry = get_tpd_from_list(cur->tok_string);
    if (tab_entry == NULL) {
        rc = TABLE_NOT_EXIST;
        cur->tok_value = INVALID;
        return rc;
    }

    cd_entry *cd_entries = NULL;
    get_cd_entries(tab_entry, &cd_entries);

    cur = cur->next;
    if (cur->tok_value != K_SET) {
        rc = INVALID_STATEMENT;
        cur->tok_value = INVALID;
        return rc;
    }

    // update column
    cur = cur->next;
    col_item update_col;
    memset(&update_col, '\0', sizeof(col_item));

    // check for column class
    if (cur->tok_class != identifier) {
        rc = INVALID_COLUMN_NAME;
        cur->tok_value = INVALID;
        return rc;
    }

    int col_index = get_cd_entry_index(cd_entries, tab_entry->num_columns, cur->tok_string);
    // column name not found
    if (col_index < 0) {
        rc = INVALID_COLUMN_NAME;
        cur->tok_value = INVALID;
        return rc;
    }
    update_col.col_id = col_index;
    update_col.token = cur;

    cur = cur->next;
    if (cur->tok_value != S_EQUAL) {
        rc = INVALID_STATEMENT;
        cur->tok_value = INVALID;
        return rc;
    }

    // update value
    cur = cur->next;
    update_col.is_null = false;
    switch (cur->tok_value) {
        case K_NULL: {
            if (update_col.is_null && cd_entries[col_index].not_null) {
                rc = INVALID_COLUMN_DATA;
                return rc;
            }
            update_col.is_null = true;
            break;
        }

        case INT_LITERAL: {
            if (cd_entries[col_index].col_type != T_INT) {
                rc = INVALID_COLUMN_DATA;
            }
            update_col.int_val = atoi(cur->tok_string);
            break;
        }

        case STRING_LITERAL: {
            if (cd_entries[col_index].col_type == T_INT) {
                rc = INVALID_COLUMN_DATA;
            }
            strcpy(update_col.string_val, cur->tok_string);
            break;
        }
        default: {
            rc = INVALID;
            break;
        }
    }
    if (rc) {
        cur->tok_value = INVALID;
        return rc;
    }

    bool has_where_clause = false;
    row_predicate row_filter;
    memset(&row_filter, '\0', sizeof(row_predicate));
    row_filter.type = K_AND;

    cur = cur->next;
    rc = parse_where_clauses(cur, has_where_clause, cd_entries, tab_entry, row_filter);
    if (rc) {
        return rc;
    }

    if (cur->tok_value != EOC) {
        rc = INVALID_STATEMENT;
        cur->tok_value = INVALID;
        return rc;
    }

    table_file_header *tab_header = NULL;
    if ((rc = load_table_file(tab_entry, &tab_header)) != 0) {
        return rc;
    }

    // records
    char *records = NULL;
    if (tab_header->file_size > tab_header->record_offset) {
        records = ((char *)tab_header) + tab_header->record_offset;
    }
    row_item rows[tab_header->num_records];
    memset(rows, '\0', sizeof(rows));
    row_item *p_current_row = NULL;
    int num_loaded_records = 0;
    int num_affected_records = 0;

    for (int i = 0; i < tab_header->num_records; i++) {
        p_current_row = &rows[num_loaded_records];
        update_row_item(cd_entries, tab_entry->num_columns, p_current_row,
                        records);
        num_loaded_records++;

        // Update records.
        if ((!has_where_clause) ||
            is_row_filtered(cd_entries, tab_entry->num_columns, p_current_row,
                            &row_filter)) {
            bool value_changed = false;
            if (cd_entries[update_col.col_id].col_type == T_INT) {
                if (update_col.is_null) {
                    if (!p_current_row->value_ptrs[update_col.col_id]->is_null) {
                        p_current_row->value_ptrs[update_col.col_id]->is_null = true;
                        p_current_row->value_ptrs[update_col.col_id]->int_val = 0;
                        value_changed = true;
                    }
                } else {
                    if (p_current_row->value_ptrs[update_col.col_id]->is_null ||
                        p_current_row->value_ptrs[update_col.col_id]->int_val !=
                            update_col.int_val) {
                        p_current_row->value_ptrs[update_col.col_id]->is_null = false;
                        p_current_row->value_ptrs[update_col.col_id]->int_val =
                            update_col.int_val;
                        value_changed = true;
                    }
                }
            } else {
                if (update_col.is_null) {
                    if (!p_current_row->value_ptrs[update_col.col_id]->is_null) {
                        p_current_row->value_ptrs[update_col.col_id]->is_null = true;
                        memset(
                            p_current_row->value_ptrs[update_col.col_id]->string_val,
                            '\0', MAX_STRING_LEN + 1);
                        value_changed = true;
                    }
                } else {
                    if (p_current_row->value_ptrs[update_col.col_id]->is_null ||
                        strcmp(p_current_row->value_ptrs[update_col.col_id]
                                   ->string_val,
                               update_col.string_val) != 0) {
                        p_current_row->value_ptrs[update_col.col_id]->is_null = false;
                        strcpy(
                            p_current_row->value_ptrs[update_col.col_id]->string_val,
                            update_col.string_val);
                        value_changed = true;
                    }
                }
            }
            if (value_changed) {
                copy_columns_as_bytes(cd_entries, p_current_row->value_ptrs,
                                      tab_entry->num_columns, records,
                                      tab_header->record_size);
                num_affected_records++;
            }
        }
        records += tab_header->record_size;
    }

    // write to table file
    if (num_affected_records > 0) {
        char table_filename[MAX_IDENT_LEN + 5];
        sprintf(table_filename, "%s.tab", tab_header->tpd_ptr->table_name);
        FILE *fhandle = NULL;
        if ((fhandle = fopen(table_filename, "wbc")) == NULL) {
            rc = FILE_OPEN_ERROR;
        } else {
            tab_header->tpd_ptr = NULL;  // Reset tpd pointer.
            fwrite(tab_header, tab_header->file_size, 1, fhandle);
            fflush(fhandle);
            fclose(fhandle);
        }
    }

    free(tab_header);
    printf("%d rows are update", num_affected_records);
    return rc;
}

int parse_where_clauses(token_list *&cur, bool &has_where_clause, cd_entry cd_entries[], tpd_entry *tab_entry, row_predicate &row_filter) {
    int rc = 0;
    // WHere clause
    if (cur->tok_value == K_WHERE) {
        has_where_clause = true;
        int col_index = -1;
        int num_conditions = 0;
        bool has_more_condition = true;

        while (has_more_condition) {
            cur = cur->next;
            // check for column class
            if (cur->tok_class != identifier) {
                rc = INVALID_COLUMN_NAME;
                cur->tok_value = INVALID;
                return rc;
            }

            // find the column
            col_index = get_cd_entry_index(cd_entries, tab_entry->num_columns, cur->tok_string);
            if (col_index == -1) {
                rc = INVALID_COLUMN_NAME;
                cur->tok_value = INVALID;
                return rc;
            }
            // add column condtion for the row
            row_filter.conditions[num_conditions].col_id = col_index;

            // parse for the operator
            cur = cur->next;
            if (cur->tok_value == S_LESS || cur->tok_value == S_GREATER ||
                cur->tok_value == S_EQUAL) {
                row_filter.conditions[num_conditions].op_type = cur->tok_value;
                cur = cur->next;
                // get the operand
                if (cur->tok_value == INT_LITERAL && cd_entries[col_index].col_type == T_INT) {
                    row_filter.conditions[num_conditions].int_data_value = atoi(cur->tok_string);
                } else if (cur->tok_value == STRING_LITERAL && cd_entries[col_index].col_type != T_INT) {
                    strcpy(row_filter.conditions[num_conditions].string_data_value, cur->tok_string);
                } else {
                    rc = INVALID_CONDITION_OPERAND;
                    cur->tok_value = INVALID;
                    return rc;
                }
            } else if (cur->tok_value == K_IS &&
                       cur->next->tok_value == K_NULL) {
                //    one step for NULL
                cur = cur->next;
                row_filter.conditions[num_conditions].op_type = K_IS;
            } else if (cur->tok_value == K_IS && cur->next->tok_value == K_NOT &&
                       cur->next->next->tok_value == K_NULL) {
                //    Two steps for NOT and NULL
                cur = cur->next->next;
                row_filter.conditions[num_conditions].op_type = K_NOT;
            } else {
                rc = INVALID_CONDITION;
                cur->tok_value = INVALID;
                return rc;
            }

            num_conditions++;
            row_filter.num_conditions = num_conditions;

            // requirement of max 2 conditions
            if (num_conditions == MAX_NUM_CONDITION) {
                break;
            }

            if (cur->next->tok_value == K_AND || cur->next->tok_value == K_OR) {
                cur = cur->next;
                has_more_condition = true;
                row_filter.type = cur->tok_value;
            } else {
                has_more_condition = false;
            }
        }
        cur = cur->next;
    }
    return rc;
}
int copy_columns_as_bytes(cd_entry cd_entries[], col_item *col_items[],
                          int num_cols, char record_bytes[],
                          int num_record_bytes) {
    memset(record_bytes, '\0', num_record_bytes);
    unsigned char value_length = 0;
    int cur_offset_in_record = 0;
    int int_value = 0;
    char *string_value = NULL;
    for (int i = 0; i < num_cols; i++) {
        if (cd_entries[i].col_type == T_INT) {
            // Store a integer.
            if (col_items[i]->is_null) {
                // Null value.
                value_length = 0;
                memcpy(record_bytes + cur_offset_in_record, &value_length, 1);
                cur_offset_in_record += (1 + cd_entries[i].col_len);
            } else {
                // Integer value.
                int_value = col_items[i]->int_val;
                value_length = cd_entries[i].col_len;
                memcpy(record_bytes + cur_offset_in_record, &value_length, 1);
                cur_offset_in_record += 1;
                memcpy(record_bytes + cur_offset_in_record, &int_value, value_length);
                cur_offset_in_record += cd_entries[i].col_len;
            }
        } else {
            // Store a string.
            if (col_items[i]->is_null) {
                // Null value.
                value_length = 0;
                memcpy(record_bytes + cur_offset_in_record, &value_length, 1);
                cur_offset_in_record += (1 + cd_entries[i].col_len);
            } else {
                // String value.
                string_value = col_items[i]->string_val;
                value_length = strlen(string_value);
                memcpy(record_bytes + cur_offset_in_record, &value_length, 1);
                cur_offset_in_record += 1;
                memcpy(record_bytes + cur_offset_in_record, string_value,
                       strlen(string_value));
                cur_offset_in_record += cd_entries[i].col_len;
            }
        }
    }
    return cur_offset_in_record;
}

int validate_columns(int n_columns, col_item col_items[], cd_entry cd_entries[]) {
    int rc = 0;

    // TODO:
    int type = K_NULL;
    for (int i = 0; i < n_columns; i++) {
        type = col_items[i].token->tok_value;
        if (cd_entries[i].not_null && col_items[i].is_null) {
            rc = INVALID_COLUMN_DATA;
            col_items[i].token->tok_value = INVALID;
            break;
        } else if (type == INT_LITERAL && cd_entries[i].col_type != T_INT) {
            col_items[i].token->tok_value = INVALID;
            rc = INVALID_COLUMN_DATA;
            break;
        } else if (type == STRING_LITERAL && ((cd_entries[i].col_type != T_CHAR) ||
                                              (cd_entries[i].col_len <
                                               (int)strlen(col_items[i].string_val)))) {
            col_items[i].token->tok_value = INVALID;
            rc = INVALID_COLUMN_DATA;
            break;
        }
    }
    return rc;
}

int load_table_file(tpd_entry *tab_entry, table_file_header **table_header) {
    int rc = 0;
    char table_filename[MAX_IDENT_LEN + 5];
    sprintf(table_filename, "%s.tab", tab_entry->table_name);

    FILE *fhandle = NULL;
    if ((fhandle = fopen(table_filename, "rbc")) == NULL) {
        return FILE_OPEN_ERROR;
    }
    int file_size = get_file_size(fhandle);
    table_file_header *tab_header = (table_file_header *)malloc(file_size);
    fread(tab_header, file_size, 1, fhandle);
    fclose(fhandle);
    if (tab_header->file_size != file_size) {
        rc = FILE_OPEN_ERROR;
        free(tab_header);
    }
    tab_header->tpd_ptr = tab_entry;
    *table_header = tab_header;
    return rc;
}

int get_file_size(FILE *fhandle) {
    if (!fhandle) {
        return -1;
    }
    struct stat file_stat;
    fstat(fileno(fhandle), &file_stat);
    return (int)(file_stat.st_size);
}

int get_cd_entry_index(cd_entry cd_entries[], int num_cols, char *col_name) {
    for (int i = 0; i < num_cols; i++) {
        // Column names are case-insensitive.
        if (strcasecmp(cd_entries[i].col_name, col_name) == 0) {
            return i;
        }
    }
    return -1;
}

int update_row_item(cd_entry cd_entries[], int num_cols, row_item *p_row,
                    char record_bytes[]) {
    memset(p_row, '\0', sizeof(row_item));

    int offset_in_record = 0;
    unsigned char value_length = 0;
    col_item *p_col_item = NULL;
    for (int i = 0; i < num_cols; i++) {
        // Get value length.
        memcpy(&value_length, record_bytes + offset_in_record, 1);
        offset_in_record += 1;

        // Get field value.
        p_col_item = (col_item *)malloc(sizeof(col_item));
        p_col_item->col_id = cd_entries[i].col_id;
        p_col_item->is_null = (value_length == 0);
        p_col_item->token = NULL;
        if (cd_entries[i].col_type == T_INT && !p_col_item->is_null) {
            // Set an integer.
            p_col_item->type = T_INT;
            memcpy(&p_col_item->int_val, record_bytes + offset_in_record,
                   value_length);
        } else if (!p_col_item->is_null) {
            // Set a string.
            p_col_item->type = T_VARCHAR;
            memcpy(p_col_item->string_val, record_bytes + offset_in_record,
                   value_length);
            p_col_item->string_val[value_length] = '\0';
        }
        p_row->value_ptrs[i] = p_col_item;
        offset_in_record += cd_entries[i].col_len;
    }
    p_row->num_fields = num_cols;
    p_row->sorting_col_id = -1;
    p_row->next = NULL;
    return offset_in_record;
}

void print_table_border(cd_entry *list_cd_entries[], int num_values) {
    int col_width = 0;
    for (int i = 0; i < num_values; i++) {
        printf("-");
        col_width = column_display_width(list_cd_entries[i]);
        repeat_print_char('-', col_width + 2);
    }
    printf("-\n");
}

void print_table_column_names(cd_entry *list_cd_entries[],
                              col_info col_infos[], int num_values) {
    int col_gap = 0;
    for (int i = 0; i < num_values; i++) {
        printf("%c %s", '|', col_infos[i].name);
        col_gap = column_display_width(list_cd_entries[i]) -
                  strlen(list_cd_entries[i]->col_name) + 1;
        repeat_print_char(' ', col_gap);
    }
    printf("%c\n", '|');
}

int column_display_width(cd_entry *col_entry) {
    int col_name_len = strlen(col_entry->col_name);
    if (col_entry->col_len > col_name_len) {
        return col_entry->col_len;
    } else {
        return col_name_len;
    }
}

void print_record_row(cd_entry *list_cd_entries[], int num_cols,
                      row_item *row) {
    int col_gap = 0;
    char display_value[MAX_STRING_LEN + 1];
    int col_index = -1;
    col_item **col_items = row->value_ptrs;
    bool left_align = true;
    for (int i = 0; i < num_cols; i++) {
        col_index = list_cd_entries[i]->col_id;
        left_align = true;
        if (!col_items[col_index]->is_null) {
            if (list_cd_entries[i]->col_type == T_INT) {
                left_align = false;
                sprintf(display_value, "%d", col_items[col_index]->int_val);
            } else {
                strcpy(display_value, col_items[col_index]->string_val);
            }
        } else {
            // Display NULL value as a dash.
            strcpy(display_value, "-");
            left_align = (list_cd_entries[i]->col_type == STRING_LITERAL);
        }
        col_gap =
            column_display_width(list_cd_entries[i]) - strlen(display_value) + 1;
        if (left_align) {
            printf("| %s", display_value);
            repeat_print_char(' ', col_gap);
        } else {
            printf("|");
            repeat_print_char(' ', col_gap);
            printf("%s ", display_value);
        }
    }
    printf("%c\n", '|');
}

void print_aggregate_result(int aggregate_type, int num_fields,
                            int records_count, int int_sum,
                            cd_entry *list_cd_entries[]) {
    char display_value[MAX_STRING_LEN + 1];
    memset(display_value, '\0', sizeof(display_value));
    if (aggregate_type == F_SUM) {
        sprintf(display_value, "%d", int_sum);
    } else if (aggregate_type == F_AVG) {
        if (records_count == 0) {
            sprintf(display_value, "NaN");
        } else {
            sprintf(display_value, "%d", int_sum / records_count);
        }
    } else if (aggregate_type == F_COUNT) {
        sprintf(display_value, "%d", records_count);
    }

    char display_title[MAX_STRING_LEN + 1];
    memset(display_title, '\0', sizeof(display_title));
    if (num_fields == 1) {
        // Aggregate on one column.
        if (aggregate_type == F_SUM) {
            sprintf(display_title, "SUM(%s)", list_cd_entries[0]->col_name);
        } else if (aggregate_type == F_AVG) {
            sprintf(display_title, "AVG(%s)", list_cd_entries[0]->col_name);
        } else {  // F_COUNT
            sprintf(display_title, "COUNT(%s)", list_cd_entries[0]->col_name);
        }
    } else {
        // Aggregate on one column.
        sprintf(display_title, "COUNT(*)");
    }

    int display_width = (strlen(display_value) > strlen(display_title))
                            ? strlen(display_value)
                            : strlen(display_title);
    printf("-");
    repeat_print_char('-', display_width + 2);
    printf("-\n");

    printf("| %s ", display_title);
    repeat_print_char(' ', display_width - strlen(display_title));
    printf("|\n");

    printf("-");
    repeat_print_char('-', display_width + 2);
    printf("-\n");

    printf("| %s ", display_value);
    repeat_print_char(' ', display_width - strlen(display_value));
    printf("|\n");

    printf("-");
    repeat_print_char('-', display_width + 2);
    printf("-\n");
}

bool is_row_filtered(cd_entry cd_entries[], int num_cols, row_item *row_ptr,
                     row_predicate *predicate_ptr) {
    if (!predicate_ptr || predicate_ptr->num_conditions < 1) {
        return true;
    }

    col_item *lhs_operand =
        row_ptr->value_ptrs[predicate_ptr->conditions[0].col_id];
    bool result = eval_condition(cd_entries, &predicate_ptr->conditions[0], lhs_operand);
    if (predicate_ptr->num_conditions == 1) {
        // Only one condition.
        return result;
    }

    // Evaluate the second condition.
    lhs_operand = row_ptr->value_ptrs[predicate_ptr->conditions[1].col_id];
    if (predicate_ptr->type == K_AND) {
        // AND conditions.
        result = result && eval_condition(cd_entries, &predicate_ptr->conditions[1], lhs_operand);
    } else {
        // OR conditions.
        result = result || eval_condition(cd_entries, &predicate_ptr->conditions[1], lhs_operand);
    }

    return result;
}

bool eval_condition(cd_entry cd_entries[], row_condition *condition_ptr, col_item *col_item_ptr) {
    bool result = true;
    switch (condition_ptr->op_type) {
        case S_LESS:
            // Operator "<"
            if (cd_entries[condition_ptr->col_id].col_type == T_INT) {
                if (col_item_ptr->is_null) {
                    result = false;
                } else {
                    result = (col_item_ptr->int_val < condition_ptr->int_data_value);
                }
            } else {
                if (col_item_ptr->is_null) {
                    result = false;
                } else {
                    result = (strcmp(col_item_ptr->string_val,
                                     condition_ptr->string_data_value) < 0);
                }
            }
            break;
        case S_EQUAL:
            // Operator "="
            if (cd_entries[condition_ptr->col_id].col_type == T_INT) {
                if (col_item_ptr->is_null) {
                    result = false;
                } else {
                    result = (col_item_ptr->int_val == condition_ptr->int_data_value);
                }
            } else {
                if (col_item_ptr->is_null) {
                    result = false;
                } else {
                    result = (strcmp(col_item_ptr->string_val,
                                     condition_ptr->string_data_value) == 0);
                }
            }
            break;
        case S_GREATER:
            // Operator ">"
            if (cd_entries[condition_ptr->col_id].col_type == T_INT) {
                if (col_item_ptr->is_null) {
                    result = false;
                } else {
                    result = (col_item_ptr->int_val > condition_ptr->int_data_value);
                }
            } else {
                if (col_item_ptr->is_null) {
                    result = false;
                } else {
                    result = (strcmp(col_item_ptr->string_val,
                                     condition_ptr->string_data_value) > 0);
                }
            }
            break;
        case K_IS:
            // Operator "IS_NULL"
            result = col_item_ptr->is_null;
            break;
        case K_NOT:
            // Operator "IS_NOT_NULL"
            result = !col_item_ptr->is_null;
            break;
        default:
            // Return true for unknown relational operators.
            result = true;
    }
    return result;
}

void sort_records(row_item rows[], int num_records, cd_entry *cd_entries_list_ptr,
                  bool is_desc) {
    for (int i = 0; i < num_records; i++) {
        rows[i].sorting_col_id = cd_entries_list_ptr->col_id;
    }
    qsort(rows, num_records, sizeof(row_item), records_comparator);
    row_item temp_record;
    if (is_desc) {
        for (int i = 0; i < num_records / 2; i++) {
            // temp_record := rows[i];
            memcpy(&temp_record, &rows[i], sizeof(row_item));
            // rows[i] := rows[num_records - 1 - i];
            memcpy(&rows[i], &rows[num_records - 1 - i], sizeof(row_item));
            // rows[num_records - 1 - i] := temp_record;
            memcpy(&rows[num_records - 1 - i], &temp_record, sizeof(row_item));
        }
    }
}

int records_comparator(const void *arg1, const void *arg2) {
    row_item *p_record1 = (row_item *)arg1;
    row_item *p_record2 = (row_item *)arg2;
    int sorting_col_id = p_record1->sorting_col_id;

    // If result < 0, elem1 less than elem2;
    // If result = 0, elem1 equivalent to elem2;
    // If result > 0, elem1 greater than elem2.
    int result = 0;
    col_item *p_value1 = p_record1->value_ptrs[sorting_col_id];
    col_item *p_value2 = p_record2->value_ptrs[sorting_col_id];
    if (p_value1->type == T_INT) {
        // Compare 2 integers.
        // NULL is smaller than any other values.
        if (p_value1->is_null) {
            result = (p_value2->is_null ? 0 : -1);
        } else {
            if (p_value2->is_null) {
                result = 1;
            } else if (p_value1->int_val < p_value2->int_val) {
                result = -1;
            } else if (p_value1->int_val > p_value2->int_val) {
                result = 1;
            } else {
                result = 0;
            }
        }
    } else {
        // Compare 2 strings.
        // NULL is smaller than any other values.
        if (p_value1->is_null) {
            result = (p_value2->is_null ? 0 : -1);
        } else {
            if (p_value2->is_null) {
                result = 1;
            } else {
                result = strcasecmp(p_value1->string_val, p_value2->string_val);
            }
        }
    }
    return result;
}

void free_row_item(row_item *row, bool to_last) {
    row_item *current_row = row;
    row_item *temp = NULL;
    while (current_row) {
        for (int i = 0; i < current_row->num_fields; i++) {
            free(current_row->value_ptrs[i]);
        }
        if (!to_last) {
            return;
        }
        temp = current_row;
        current_row = current_row->next;
        free(temp);
    }
}

int save_records_to_file(table_file_header *const tab_header,
                         row_item *const rows_head) {
    int rc = 0;
    tpd_entry *const tab_entry = tab_header->tpd_ptr;

    int num_rows = 0;
    row_item *current_row = rows_head;
    while (current_row) {
        num_rows++;
        current_row = current_row->next;
    }

    tab_header->num_records = num_rows;
    tab_header->file_size =
        sizeof(table_file_header) + tab_header->record_size * num_rows;
    tab_header->tpd_ptr = NULL;

    char table_filename[MAX_IDENT_LEN + 5];
    sprintf(table_filename, "%s.tab", tab_entry->table_name);
    FILE *fhandle = NULL;
    if ((fhandle = fopen(table_filename, "wbc")) == NULL) {
        rc = FILE_OPEN_ERROR;
    } else {
        fwrite(tab_header, tab_header->record_offset, 1, fhandle);

        char *record_raw_bytes = (char *)malloc(tab_header->record_size);
        cd_entry *cd_entries = NULL;
        get_cd_entries(tab_entry, &cd_entries);
        current_row = rows_head;
        while (current_row) {
            memset(record_raw_bytes, '\0', tab_header->record_size);
            copy_columns_as_bytes(cd_entries, current_row->value_ptrs,
                                  tab_entry->num_columns, record_raw_bytes,
                                  tab_header->record_size);
            fwrite(record_raw_bytes, tab_header->record_size, 1, fhandle);
            current_row = current_row->next;
        }
        free(record_raw_bytes);
        fflush(fhandle);
        fclose(fhandle);
    }
    return rc;
}

// int print_group_by_aggregate_result(int aggregate_type, int wildcard_col_idx) {
//     char display_title[MAX_STRING_LEN + 1];
//     memset(display_title, '\0', sizeof(display_title));
//     if (wildcard_col_idx == -1) {
//         // Aggregate on one column.
//         if (aggregate_type == F_SUM) {
//             sprintf(display_title, "SUM(%s)", list_cd_entries[0]->col_name);
//         } else if (aggregate_type == F_AVG) {
//             sprintf(display_title, "AVG(%s)", list_cd_entries[0]->col_name);
//         } else {  // F_COUNT
//             sprintf(display_title, "COUNT(%s)", list_cd_entries[0]->col_name);
//         }
//     } else {
//         // Aggregate on one column.
//         sprintf(display_title, "COUNT(*)");
//     }

//     int display_width = (strlen(display_value) > strlen(display_title))
//                             ? strlen(display_value)
//                             : strlen(display_title);
//     printf("-");
//     repeat_print_char('-', display_width + 2);
//     printf("-\n");

//     printf("| %s ", display_title);
//     repeat_print_char(' ', display_width - strlen(display_title));
//     printf("|\n");

//     printf("-");
//     repeat_print_char('-', display_width + 2);
//     printf("-\n");
// }

tpd_entry *get_tpd_from_list(char *tabname) {
    tpd_entry *tpd = NULL;
    tpd_entry *cur = &(g_tpd_list->tpd_start);
    int num_tables = g_tpd_list->num_tables;
    bool found = false;

    if (num_tables > 0) {
        while ((!found) && (num_tables-- > 0)) {
            if (strcasecmp(cur->table_name, tabname) == 0) {
                found = true;
                tpd = cur;
            } else {
                if (num_tables > 0) {
                    cur = (tpd_entry *)((char *)cur + cur->tpd_size);
                }
            }
        }
    }

    return tpd;
}