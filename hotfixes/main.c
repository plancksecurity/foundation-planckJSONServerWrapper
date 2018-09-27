#define _EXPORT_PEP_ENGINE_DLL

#include "platform.h"
#include "pEp_internal.h"

#include <limits.h>

#include "wrappers.h"

#define _GPGERR(X) ((X) & 0xffffL)

static void *gpgme;
static struct gpg_s gpg;

struct _str_ptr_and_bit {
    const char* key;
    int bit;
};

typedef struct _str_ptr_and_bit str_ptr_and_bit;

int strptrcmp(const void* a, const void* b) {
    return (int)((((str_ptr_and_bit*)(a))->key) - (((str_ptr_and_bit*)(b))->key));
}


bool quickfix_config(stringlist_t* keys, const char* config_file_path) {
    static char buf[MAX_LINELENGTH];
    size_t num_keys = stringlist_length(keys);

    // This function does:
    // 1. find a non-existent backup name numbered from 0 to 99 (otherwise fails)
    // 2. read the original file and meanwhile write the backup copy
    // 3. write the new config file to a temporary file in the same directory
    // 4. rename the temp file replacing the original config file
    // 5. on Windows remove the left-overs

    /* Find a suitable backup file name, without trashing previous ones */
    char* backup_file_path = NULL;
    size_t backup_file_path_baselen = strlen(config_file_path);
    FILE *backup_file = 0;
    int ret;
    int found = 0;
    int i;
    char* temp_config_file_path = NULL;
    char* s = NULL;
    stringlist_t* _k;
    stringlist_t* lines = new_stringlist(NULL);
    FILE *f;
    FILE *temp_config_file;
    stringlist_t* cur_string;
    bool status = false;

#ifdef WIN32
    WIN32_FIND_DATA FindFileData;
    HANDLE handle;

    const char* line_end = "\r\n";
#else
    const char* line_end = "\n";
#endif

    // If we bork it up somehow, we don't go beyond 100 tries...
    for (int nr = 0; nr < 99; nr++) {
        backup_file_path = (char*)calloc(backup_file_path_baselen + 12, 1);  // .99.pep.old\0
        ret = snprintf(backup_file_path, backup_file_path_baselen + 12,
                                        "%s.%d.pep.bkp", config_file_path, nr);
        assert(ret >= 0);  // snprintf(2)
        if (ret < 0) {
            goto quickfix_error;  // frees backup_file_path
        }

#ifdef WIN32
        // The fopen(.., "x") is not documented on Windows (fopen_s actually respects it, but...).
        // So we make an extra check for the existence of the file. This introduces a possible
        // race-condition, but it has little effect even if we incur into it.
        handle = FindFirstFile(backup_file_path, &FindFileData);
        if (handle != INVALID_HANDLE_VALUE) {
                FindClose(handle);
                free(backup_file_path);
                backup_file_path = NULL;
                continue;
        }
        FindClose(handle);

        backup_file = Fopen(backup_file_path, "wb");
#else
        backup_file = Fopen(backup_file_path, "wbx");    // the 'x' is important
#endif
        if (backup_file <= 0) {
            free(backup_file_path);
            backup_file_path = NULL;
            continue;
        }
        break;
    }

    if (!backup_file_path)
        goto quickfix_error;

    if (backup_file <= 0)
        goto quickfix_error;

    // Open original file, parse it, and meanwhile write a backup copy

    f = Fopen(config_file_path, "rb");

    if (f == NULL)
        goto quickfix_error;

    ret = Fprintf(backup_file, "# Backup created by pEp.%s"
                               "# If GnuPG and pEp work smoothly this file may safely be removed.%s%s",
                               line_end, line_end, line_end);

    // Go through every line in the file
    str_ptr_and_bit *found_keys = NULL;
    while ((s = Fgets(buf, MAX_LINELENGTH, f))) {
        // pointers to the keys found in this string
        found_keys = (str_ptr_and_bit*)(calloc(num_keys, sizeof(str_ptr_and_bit)));
        int num_found_keys = 0;

        ret = Fprintf(backup_file, "%s", s);
        assert(ret >= 0);
        if (ret < 0) {
            free(found_keys);
            found_keys = NULL;
            goto quickfix_error;
        }

        char* rest;
        char* line_token = strtok_r(s, "\r\n", &rest);

        if (!line_token)
            line_token = s;

        if (*line_token == '\n' || *line_token == '\r')
            line_token = "";

        if (*line_token == '#' || *line_token == '\0') {
            stringlist_add(lines, strdup(line_token));
            continue;
        }

        bool only_key_on_line = false;
        for (_k = keys, i = 1; _k; _k = _k->next, i<<=1) {
            char* keypos = strstr(line_token, _k->value);
            if (!keypos)
                continue;
            size_t keystr_len = strlen(_k->value);
            char* nextpos = keypos + keystr_len;
            bool notkey = false;
            
            if (keypos != line_token) {
                char prevchar = *(keypos - 1);
                switch (prevchar) {
                    case '-':
                    case ':':
                    case '/':
                    case '\\':
                        notkey = true;
                        break;
                    default:
                        break;
                }
            }

            if (*nextpos && !notkey) {
                char nextchar = *nextpos;
                switch (nextchar) {
                    case '-':
                    case ':':
                    case '/':
                    case '\\':
                        notkey = true;
                        break;
                    default:
                        break;
                }
            }
            else if (line_token == keypos) {
                only_key_on_line = true;
                if (!(found & i)) {
                    found |= i;
                    stringlist_add(lines, strdup(line_token));
                    num_found_keys++;
                }
                break;
            }

            if (!notkey) {
                // Ok, it's not just the key with a null terminator. So...
                // add a pointer to the key to the list from this string
                found_keys[num_found_keys].key = keypos;
                found_keys[num_found_keys].bit = i;
                num_found_keys++;
            }

            // Check to see if there are more annoying occurences of this 
            // key in the string
            for (keypos = strstr(nextpos, _k->value); 
                keypos; keypos = strstr(nextpos, _k->value)) {
                notkey = false;
                nextpos = keypos + keystr_len;
                char prevchar = *(keypos - 1);
                switch (prevchar) {
                    case '-':
                    case ':':
                    case '/':
                    case '\\':
                        notkey = true;
                        break;
                    default:
                        break;
                }
                if (!notkey) {
                    char nextchar = *nextpos;
                    switch (nextchar) {
                        case '-':
                        case ':':
                        case '/':
                        case '\\':
                            notkey = true;
                            break;
                        default:
                            break;
                    }
                }    
                if (notkey)
                    continue;
                    
                if (num_found_keys >= num_keys)
                    found_keys = (str_ptr_and_bit*)realloc(found_keys, (num_found_keys + 1) * sizeof(str_ptr_and_bit));
                    
                found_keys[num_found_keys].key = keypos;
                found_keys[num_found_keys].bit = i;
                num_found_keys++;     
            }
        }

        if (!only_key_on_line) {
            if (num_found_keys == 0)
                stringlist_add(lines, strdup(line_token));        
            else if (num_found_keys == 1 && (line_token == found_keys[0].key)) {
                if (!(found & found_keys[0].bit)) {
                    stringlist_add(lines, strdup(line_token)); 
                    found |= found_keys[0].bit;
                }
            }
            else {
                qsort(found_keys, num_found_keys, sizeof(str_ptr_and_bit), strptrcmp);
                int j;
                const char* curr_start = line_token;
                const char* next_start = NULL;
                for (j = 0; j < num_found_keys; j++, curr_start = next_start) {
                    next_start = found_keys[j].key;
                    if (curr_start == next_start)
                        continue;
                    size_t copy_len = next_start - curr_start;
                    const char* movable_end = next_start - 1;
                    while (copy_len > 0 && (*movable_end == ' ' || *movable_end == '\t' || *movable_end == '\0')) {
                        movable_end--;
                        copy_len--;
                    }
                    if (copy_len > 0) {
                        if (j == 0 || !(found & found_keys[j - 1].bit)) {
                            // if j is 0 here, the first thing in the string wasn't a key, or we'd have continued.
                            // otherwise, regardless of the value of j, we check that the "last" key (j-1) isn't already
                            // found and dealt with.
                            // Having passed that, we copy.
                            stringlist_add(lines, strndup(curr_start, copy_len));
                            if (j > 0)
                                found |= found_keys[j-1].bit;
                        }
                    }
                }
                if (!(found & found_keys[num_found_keys - 1].bit)) {
                    stringlist_add(lines, strdup(found_keys[num_found_keys - 1].key));
                    found |= found_keys[num_found_keys - 1].bit;
                }            
            }
        }
        free(found_keys);
        found_keys = NULL;
    } // End of file

    // Now do the failsafe writing dance

    ret = Fclose(f);
    assert(ret == 0);
    if (ret != 0)
        goto quickfix_error;

    ret = Fclose(backup_file);
    assert(ret == 0);
    if (ret != 0)
        goto quickfix_error;

    // 2. Write the new config file to a temporary file in the same directory

    assert(backup_file_path_baselen != NULL);

    temp_config_file_path = (char*)calloc(backup_file_path_baselen + 8, 1);  // .XXXXXX\0
    ret = snprintf(temp_config_file_path, backup_file_path_baselen + 8, "%s.XXXXXX", config_file_path);
    assert(ret >= 0);
    if (ret < 0)
        goto quickfix_error;

    int temp_config_filedesc = Mkstemp(temp_config_file_path);
    assert(temp_config_filedesc != -1);
    if (temp_config_filedesc == -1)
        goto quickfix_error;

    temp_config_file = Fdopen(temp_config_filedesc, "wb");    // no "b" in fdopen() is documentend, use freopen()
    assert(temp_config_file != NULL);
    if (temp_config_file == NULL)
        goto quickfix_error;

    // temp_config_file = Freopen(config_file_path, "wb", temp_config_file);
    // assert(temp_config_file != NULL);
    // if (temp_config_file == NULL)
    //     goto quickfix_error;

    ret = Fprintf(temp_config_file, "# File re-created by pEp%s"
                                    "# See backup in '%s'%s%s", line_end,
                                                                backup_file_path,
                                                                line_end, line_end);
    assert(ret >= 0);
    if (ret < 0)
        goto quickfix_error;
        
    for (cur_string = lines; cur_string; cur_string = cur_string->next) {
        assert(cur_string->value != NULL);
        ret = Fprintf(temp_config_file, "%s%s", cur_string->value, line_end);
        assert(ret >= 0);
        if (ret < 0)
            goto quickfix_error;
    }

    ret = Fclose(temp_config_file);
    assert(ret == 0);
    if (ret != 0)
        goto quickfix_error;

#ifdef WIN32
    ret = !(0 == ReplaceFile(config_file_path, temp_config_file_path, NULL, 0, NULL, NULL));
    assert(ret == 0);
    if (ret != 0)
        goto quickfix_error;

    ret = unlink(temp_config_file_path);
#else
    ret = rename(temp_config_file_path, config_file_path);
    // ret = 0;
#endif
    assert(ret == 0);
    if (ret != 0)
        goto quickfix_error;

    free(temp_config_file_path);
    temp_config_file_path = NULL;

    status = true;

    free(backup_file_path);

    goto quickfix_success;


quickfix_error:

    assert(status == false);

    free(backup_file_path);


quickfix_success:

    // assert(found_keys == NULL);
    if (found_keys) {
        free(found_keys);
        found_keys = NULL;
    }

    if (temp_config_file_path)
        free(temp_config_file_path);

    free_stringlist(lines);

    return status;

}


static bool ensure_config_values(stringlist_t *keys, stringlist_t *values, const char* config_file_path)
{
    static char buf[MAX_LINELENGTH];
    int r;
    stringlist_t *_k;
    stringlist_t *_v;
    unsigned int i;
    unsigned int found = 0;
    bool eof_nl = 0;
    char * rest;
    char * token;
    char * s;
    const char* line_end;

#ifdef WIN32
    line_end = "\r\n";
#else
    line_end = "\n";
#endif    

    FILE *f = Fopen(config_file_path, "rb");
    if (f == NULL && errno == ENOMEM)
        return false;

    if (f != NULL) {
                
        int length = stringlist_length(keys);

        // make sure we 1) have the same number of keys and values
        // and 2) we don't have more key/value pairs than
        // the size of the bitfield used to hold the indices
        // of key/value pairs matching keys in the config file.
        assert(length <= sizeof(unsigned int) * CHAR_BIT);
        assert(length == stringlist_length(values));
        if (!(length == stringlist_length(values) &&
              length <= sizeof(unsigned int) * CHAR_BIT)) {
            Fclose(f);

            return false;
        }

        while ((s = Fgets(buf, MAX_LINELENGTH, f))) {
            token = strtok_r(s, " \t\r\n", &rest);
            for (_k = keys, _v = values, i = 1;
                 _k != NULL;
                 _k = _k->next, _v = _v->next, i <<= 1) {

                if (((found & i) != i) && token &&
                    (strncmp(token, _k->value, strlen(_k->value)) == 0)) {
                    found |= i;
                    break;
                }
            }
            if (feof(f)) {
                eof_nl = 1;
                break;
            }
        }

        if (!s && ferror(f))
            return false;
        f = Freopen(config_file_path, "ab", f);
    }
    else {
#ifdef WIN32
        f = Fopen(config_file_path, "wb");
#else
        f = Fopen(config_file_path, "wbx");
#endif
    }

    assert(f);
    if (f == NULL)
        return false;
    
    if (eof_nl)
        r = Fprintf(f, line_end);

    for (i = 1, _k = keys, _v = values; _k != NULL; _k = _k->next,
            _v = _v->next, i <<= 1) {
        if ((found & i) == 0) {
            r = Fprintf(f, "%s %s%s", _k->value, _v->value, line_end);
            assert(r >= 0);
            if (r < 0)
                return false;
        }
    }

    r = Fclose(f);
    assert(r == 0);
    if (r != 0)
        return false;

    return true;
}


int main(int argc, char** argv)
{

    PEP_STATUS status = PEP_STATUS_OK;
    bool bResult;

#if defined(WIN32) || defined(NDEBUG)
    printf("gpg_conf %s\n", gpg_conf());
    printf("gpg_agent_conf %s\n", gpg_agent_conf());
#else
    printf("gpg_conf %s\n", gpg_conf(false));
    printf("gpg_agent_conf %s\n", gpg_agent_conf(false));
#endif

    stringlist_t *conf_keys   = new_stringlist("keyserver");
    stringlist_t *conf_values = new_stringlist("hkp://keys.gnupg.net");

    stringlist_add(conf_keys, "cert-digest-algo");
    stringlist_add(conf_values, "SHA256");

    stringlist_add(conf_keys, "no-emit-version");
    stringlist_add(conf_values, "");

    stringlist_add(conf_keys, "no-comments");
    stringlist_add(conf_values, "");

    stringlist_add(conf_keys, "personal-cipher-preferences");
    stringlist_add(conf_values, "AES AES256 AES192 CAST5");

    stringlist_add(conf_keys, "personal-digest-preferences");
    stringlist_add(conf_values, "SHA256 SHA512 SHA384 SHA224");

    stringlist_add(conf_keys, "ignore-time-conflict");
    stringlist_add(conf_values, "");

    stringlist_add(conf_keys, "allow-freeform-uid");
    stringlist_add(conf_values, "");

    bResult = true;
    if (1)
#if defined(WIN32) || defined(NDEBUG)
        bResult = quickfix_config(conf_keys, gpg_conf());
    if (bResult)
        bResult = ensure_config_values(conf_keys, conf_values, gpg_conf());
#else
        bResult = quickfix_config(conf_keys, gpg_conf(false));
    if (bResult)
        bResult = ensure_config_values(conf_keys, conf_values, gpg_conf(false));
#endif
    status = PEP_STATUS_OK;
    
    free_stringlist(conf_keys);
    free_stringlist(conf_values);

    assert(bResult);
    if (!bResult) {
        status = PEP_INIT_NO_GPG_HOME;
        goto pep_error;
    }

    conf_keys = new_stringlist("default-cache-ttl");
    conf_values = new_stringlist("300");

    stringlist_add(conf_keys, "max-cache-ttl");
    stringlist_add(conf_values, "1200");

    if (1)
#if defined(WIN32) || defined(NDEBUG)
        bResult = quickfix_config(conf_keys, gpg_agent_conf());
    if (bResult)
        bResult = ensure_config_values(conf_keys, conf_values, gpg_agent_conf());
 #else        
        bResult = quickfix_config(conf_keys, gpg_agent_conf(false));
    if (bResult)
        bResult = ensure_config_values(conf_keys, conf_values, gpg_agent_conf(false));
 #endif
    free_stringlist(conf_keys);
    free_stringlist(conf_values);

    return 0;


pep_error:

    return 1;
    

}


