#include "gretchen.internal.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// binary file and related methods

// http://www.cse.yorku.ca/~oz/hash.html
uint64_t hash_djb2(uint8_t *str)
{ 
    uint64_t hash = 5381;
    uint64_t c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

//https://stackoverflow.com/questions/2029103/correct-way-to-read-a-text-file-into-a-buffer-in-c#2029227
uint8_t* read_binaryfile(uint8_t* filename, int64_t* size, int8_t* error)
{
    *size = -1;
    *error = 0;
    uint8_t* source = NULL;
    FILE *fp = fopen((char*) filename, "rb");
    if (fp==NULL) {
        *error = -1;
    } else {
        // go to end of file
        if (fseek(fp, 0L, SEEK_END)==0) {
            *size = ftell(fp);
            if (*size==-1) {
                *error = ferror(fp);
            }
            // allocate
            source = malloc(sizeof(uint8_t) * *size+1);
            // go to start
            if (fseek(fp, 0L, SEEK_SET)!=0) {
                *error = ferror(fp);
            }
            // read the entire file
            size_t nlen = fread(source, sizeof(uint8_t), *size, fp);  
            if (ferror(fp)!=0) {
                *error = ferror(fp);
            } else {
                source[nlen++] = '\0';
            }
        } 
        fclose(fp);
    }
    return source;
}

void optain_binaryfile_size(uint8_t* filename, int64_t* size, int8_t* error)
{
    *size = -1;
    *error = 0;
    FILE *fp = fopen((char*) filename, "rb");
    if (fp==NULL) {
        *error = -1;
    } else {
        if (fseek(fp, 0L, SEEK_END)==0) {
            *size = ftell(fp);
            if (*size==-1) {
                *error = ferror(fp);
            }
        }
    }
    fclose(fp);
}

// https://stackoverflow.com/questions/17598572/read-write-to-binary-files-in-c#17598785
void write_binaryfile(uint8_t* filename, uint8_t* source, int8_t* error)
{
    *error = 0;
    FILE *wf = fopen((char*) filename, "wb");
    if (wf==NULL) {
        *error = -1;
        return ;
    }
    fwrite(source, sizeof(uint8_t), strlen((char*) source)+1, wf);
    if (ferror(wf)!=0)
        *error = ferror(wf);
    fclose(wf);
}

void write_rawfile(uint8_t* filename, float* source, size_t len, int8_t* error)
{
    *error = 0;
    FILE *wf = fopen((char*) filename, "wb");
    if (wf==NULL) {
        *error = -1;
        return ;
    }
    fwrite(source, sizeof(float), len, wf);
    if (ferror(wf)!=0)
        *error = ferror(wf);
    fclose(wf);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// envelope methods

envelope_t* envelope_create(uint8_t* name, uint8_t* source)
{
    // check if name is NULL
    char* _name;
    if (name == NULL) {
        _name = malloc(sizeof(char)*6);
        strcpy(_name, "none\0");
    } else {
        _name = (char*)name;
        // check if the (file)name contains the delimiter chars
        // if so replace them with '_'
        char* del = strstr(_name, ENVELOPE_FORMAT_DELIMITER);
        if (del != NULL) {
            size_t pos = del-_name;
            for (int k=0; k < ENVELOPE_FORMAT_DELIMITER_LEN; k++) {
                _name[pos+k*sizeof(char)] = '_';
            }
        }
    }
    // FIXME
    // using linux/posix method `basename` for now here.
    // That wont work on windows and other platforms i guess...
    // So we have to use sth better
    // see https://stackoverflow.com/questions/7180293/how-to-extract-filename-from-path
    // a hint for a self implemented version of basename could be
    // see: https://stackoverflow.com/a/41949246
    char* base = basename((char*)_name);
    // allocate space for the actual envelope chars 
    char* name2 = malloc(sizeof(char)*strlen(base)+1);
    char* source2 = malloc(sizeof(char)*strlen((char*)source)+1);
    strcpy(name2, base);
    strcpy(source2, (char*)source);
    // create the actual envelope_t struct
    envelope_t *env = malloc(sizeof(envelope_t));
    env->name = (uint8_t*)name2;
    env->source = (uint8_t*)source2;
    // if name was null we need to free the substitude string
    if (name == NULL)
        free(_name);
    return env;
}

void envelope_destroy(envelope_t* env)
{
    if (env) {
        free(env->name);
        free(env->source);
        free(env);
    }
}

void envelope_pack(envelope_t* envelope, uint8_t** arg)
{
    size_t env_pack_size = strlen((char*)envelope->name) + 
            strlen((char*)envelope->source) + ENVELOPE_FORMAT_DELIMITER_LEN + 1;
    *arg = malloc(sizeof(uint8_t)*env_pack_size);
    strcpy((char*) *arg, "\0");
    snprintf((char*) *arg, 
             env_pack_size,
             ENVELOPE_FORMAT, 
             envelope->name,
             envelope->source);
}

void envelope_unpack(uint8_t* envelope, envelope_t** arg)
{
    // check if envelope string contains the delimiter chars
    // if not don't do anything; leaving `arg` as it is
    char* delim = strstr((char*)envelope, ENVELOPE_FORMAT_DELIMITER);
    if (delim != NULL) {
        // estimate delimiter start and end position
        size_t del_pos = delim-(char*)envelope;
        size_t del_start = del_pos;
        for (int k=0; k < ENVELOPE_FORMAT_DELIMITER_LEN; k++)
            del_pos += k;
        size_t del_end = del_pos;
        size_t source_len = strlen((char*)envelope)-del_end;
        // i am tokenizing per hand (only 2 tokens so grrr strtok or strsep...)
        char *name = malloc(sizeof(char)*(del_start+1));
        memmove(name, envelope, del_start);
        name[del_start] = '\0';
        char *source = malloc(sizeof(char)*(source_len+1));
        char *source_start = (char*)envelope + sizeof(char)*(del_end+1);
        memmove(source, source_start, source_len);
        source[source_len] = '\0';
        // create the envelope 
        *arg = malloc(sizeof(envelope_t));
        (*arg)->name = (uint8_t*) name;
        (*arg)->source = (uint8_t*) source;
    }
}

void envelope_print(envelope_t*env) 
{
    printf("envelope filename: %s\nsize: %zu\np: %p \n", 
            env->name, 
            strlen((char*)env->source), 
            env->source);
}

void envelope_writeout(envelope_t* env, uint8_t* path, int8_t* error)
{
    char *name = malloc(sizeof(uint8_t)*(strlen((char*)path)+strlen((char*)env->name))+2);
    strcpy(name, (char*)path);
    // FIXME this filesystemdelimiterstuff is hardly platform independent
    // solve or factor out
    // i could require that path ends with '/' or (see above) legel delim
    strcat(name, (char*)env->name);
    write_binaryfile((uint8_t*)name, env->source, &*error);
    /*printf("File written: %s error:%i\n", name, *error);*/
    free(name);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// rx frame handling methods, chunks, transmits

transmit_t* transmit_create(uint16_t hash, uint16_t max)
{
    transmit_t* transm = malloc(sizeof(transmit_t));
    transm->hash = hash;
    transm->max = max;
    transm->chunks = malloc(sizeof(chunk_t)*max);
    for (uint32_t k=0; k<max; k++) {
        transm->chunks[k].num = k;
        transm->chunks[k].data = NULL;
        transm->chunks[k].len = 0;
    }
    return transm;
}

void transmit_destroy(transmit_t* transm)
{
    if (transm) {
        for (uint32_t k=0; k<transm->max; k++) {
            if (transm->chunks[k].data)
                free(transm->chunks[k].data);
        }
        free(transm->chunks);
        free(transm);
    }
}

void transmit_add(transmit_t* transm, uint16_t num, uint8_t* buffer, size_t buffer_len)
{
    if (num >= transm->max)
        return;
    if (!transm->chunks[num].data) {
        transm->chunks[num].data = malloc(sizeof(uint8_t)*buffer_len+1);
        memmove(transm->chunks[num].data, buffer, buffer_len); 
        transm->chunks[num].len = buffer_len;
    }    
}

bool transmit_is_complete(transmit_t* transm)
{
    bool retv = true;
    for (uint32_t k=0; k<transm->max; k++) {
        if (!transm->chunks[k].data) {
            retv = false;
            break;
        }
    }
    return retv;
}

static void transmit_concatenate(transmit_t* transm, uint8_t** arg)
{
    size_t concat_size=0;
    for (size_t k=0; k<transm->max; k++)
        concat_size += transm->chunks[k].len+1;
    *arg = malloc(sizeof(uint8_t)*2);
    strcpy((char*) *arg, "\0");
    if (!transmit_is_complete(transm))
        return ;
    *arg = realloc(*arg, sizeof(uint8_t)*concat_size);
    for (uint32_t k=0; k<transm->max; k++) {
        strncat((char*) *arg, (char*) transm->chunks[k].data, transm->chunks[k].len);
    }
}

void transmit_print(transmit_t* transm)
{
    if (transm == NULL)
        return;
    printf("hash %u ", transm->hash);
    printf("max %u \n", transm->max);
    printf(" is complete %i\n", transmit_is_complete(transm));
    for (uint32_t k=0; k<transm->max; k++) {
        printf(" chunk %i %p \n", k, transm->chunks[k].data); 
    }
}
// FIXME
// no checking the envelope is complete...
void transmit_create_envelope(transmit_t* transm, envelope_t** arg)
{
    uint8_t* envstr;
    transmit_concatenate(transm, &envstr);
    envelope_unpack(envstr, &*arg);
    free(envstr);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// rx frame handler

rxhandler_t* rxhandler_create()
{
    rxhandler_t* rxm = malloc(sizeof(rxhandler_t));
    rxm->transmits = hashmap_new(); 
    return rxm;
}

typedef struct {
    map_t* map;
    int32_t itrnum;
    void* user;
} _itr_helper_t;

typedef struct {
    list_cb_t* callback;
    void* user;
} _list_helper_t;

static int list_iterator(any_t item, any_t data)
{
    _itr_helper_t* itrhelper = (_itr_helper_t*) item;
    _list_helper_t* listhelper = (_list_helper_t*) itrhelper->user;
    listhelper->callback(data, listhelper->user);
    itrhelper->itrnum ++;
    if (itrhelper->itrnum < hashmap_length(itrhelper->map))
        return MAP_OK;
    else
        return MAP_MISSING;
}

void rxhandler_list(rxhandler_t* rxm, list_cb_t callback, void *user)
{
    _list_helper_t* list_helper = malloc(sizeof(_list_helper_t));
    list_helper->callback = callback;
    list_helper->user = user;
    _itr_helper_t* itr_helper = malloc(sizeof(_itr_helper_t));
    itr_helper->map = rxm->transmits;
    itr_helper->itrnum = 0;
    itr_helper->user = list_helper;
    int8_t error;
    while(true) {
        error = hashmap_iterate(rxm->transmits, list_iterator, itr_helper);
        if (error!=MAP_OK)
            break; 
    }
    free(itr_helper);
    free(list_helper);
}

static void _destroy_callback(transmit_t* transm, void* user)
{
    (void) user;
    transmit_destroy(transm);
}

void rxhandler_destroy(rxhandler_t* rxm)
{
    if (rxm) {
        rxhandler_list(rxm, _destroy_callback, NULL);
        hashmap_free(rxm->transmits);
        free(rxm);
    }
}

#define KEY_FROM_HASH(hash) \
    uint8_t key[RXMAP_KEY_LEN]; \
    memset(key, '\0', RXMAP_KEY_LEN); \
    snprintf((char*)key,\
                    RXMAP_KEY_LEN,\
                    RXMAP_KEY_FORMAT,\
                    hash);

void rxhandler_add(rxhandler_t* rxm, uint16_t hash, uint16_t num, uint16_t max, uint8_t* buffer, size_t buffer_len)
{
    transmit_t *transm = NULL;
    KEY_FROM_HASH(hash);
    int8_t error = hashmap_get(rxm->transmits, key, (any_t*)&transm);
    if (error==MAP_MISSING) {
        transm = transmit_create(hash, max);
        hashmap_put(rxm->transmits, key, (any_t*)transm); 
    }
    transmit_add(transm, num, buffer, buffer_len);
}

void rxhandler_get(rxhandler_t* rxm, uint16_t hash, transmit_t** arg)
{
    transmit_t *transm = NULL;
    KEY_FROM_HASH(hash);
    int8_t error = hashmap_get(rxm->transmits, key, (any_t*)&transm);
    if (error==MAP_OK)
        *arg = transm;
    else
        *arg = NULL;
}

void rxhandler_remove(rxhandler_t* rxm, uint16_t hash)
{
    transmit_t *transm = NULL;
    KEY_FROM_HASH(hash);
    int8_t error = hashmap_get(rxm->transmits, key, (any_t*)&transm);
    if (error==MAP_MISSING)
        return;
    transmit_destroy(transm);
    hashmap_remove(rxm->transmits, key);
}

static void _reap_callback(transmit_t* transm, void* user)
{
    if (transmit_is_complete(transm)) {
        // NOTE
        // it is returning just the first and is 
        // ignoring all other possible complete transmits
        transmit_t** out = user;
        if (*out==NULL) {
            *out = transm;
        }
    }
} 

void rxhandler_reap(rxhandler_t* rxm, transmit_t** ripe)
{
    *ripe = NULL;
    rxhandler_list(rxm, _reap_callback, ripe);
}





