/*
    goahead.h -- GoAhead Web Server Header

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#ifndef _h_GOAHEAD
#define _h_GOAHEAD 1

/************************************ Overrides *******************************/
/*
    Override osdep defaults
 */
#define ME_MAX_IP 64                /**< Maximum IP address size */

/************************************ Includes ********************************/

#include    "me.h"
#include    "osdep.h"
#include    "fcgiapp.h"

/************************************ Defaults ********************************/

#ifdef __cplusplus
extern "C" {
#endif







#if WIN32
typedef fd_set fd_mask;
#endif

#if !LINUX
PUBLIC char *basename(char *name);
#endif



/**
    File status structure
 */
typedef struct stat WebsStat;

/*

 */
#define COPYRIGHT \
    "Copyright (c) Custom Webserver"

/************************************* Main ***********************************/

#define ME_MAX_ARGC 32

PUBLIC int websParseArgs(char *args, char **argv, int maxArgc);

/**
    Emit an error message
    @return Zero if successful
    @stability Stable
*/
PUBLIC void error(char *fmt, ...);

/************************************ Tunables ********************************/

#define WEBS_MAX_LISTEN     8           /**< Maximum number of listen endpoints */
#define WEBS_SMALL_HASH     31          /**< General small hash size */
#define WEBS_MAX_PASSWORD   32          /**< Default maximum password */

/************************************* Error **********************************/


#define WEBS_L          __FILE__, __LINE__
#define WEBS_ARGS_DEC   char *file, int line
#define WEBS_ARGS       file, line

PUBLIC_DATA int logLevel;

/**
    Standard logging trace levels are 0 to 9 with 0 being the most verbose. These are ored with the error source
    and type flags. The WEBS_LOG_MASK is used to extract the trace level from a flags word. We expect most apps
    to run with level 2 trace enabled.
*/
#define WEBS_ERROR          1           /**< Hard error trace level */
#define WEBS_WARN           2           /**< Soft warning trace level */
#define WEBS_CONFIG         2           /**< Configuration settings trace level. */
#define WEBS_VERBOSE        9           /**< Highest level of trace */
#define WEBS_LEVEL_MASK     0xF         /**< Level mask */

/*
    Log message flags
 */
#define WEBS_ASSERT_MSG     0x10        /**< Originated from assert */
#define WEBS_ERROR_MSG      0x20        /**< Originated from error */
#define WEBS_LOG_MSG        0x100       /**< Originated from logmsg */
#define WEBS_RAW_MSG        0x200       /**< Raw message output */
#define WEBS_TRACE_MSG      0x400       /**< Originated from trace */




/*********************************** HTTP Codes *******************************/
/*
    Standard HTTP/1.1 status codes
 */
#define HTTP_CODE_CONTINUE                  100     /**< Continue with request, only partial content transmitted */
#define HTTP_CODE_OK                        200     /**< The request completed successfully */
#define HTTP_CODE_CREATED                   201     /**< The request has completed and a new resource was created */
#define HTTP_CODE_ACCEPTED                  202     /**< The request has been accepted and processing is continuing */
#define HTTP_CODE_NOT_AUTHORITATIVE         203     /**< The request has completed but content may be from another source */
#define HTTP_CODE_NO_CONTENT                204     /**< The request has completed and there is no response to send */
#define HTTP_CODE_RESET                     205     /**< The request has completed with no content. Client must reset view */
#define HTTP_CODE_PARTIAL                   206     /**< The request has completed and is returning partial content */
#define HTTP_CODE_MOVED_PERMANENTLY         301     /**< The requested URI has moved permanently to a new location */
#define HTTP_CODE_MOVED_TEMPORARILY         302     /**< The URI has moved temporarily to a new location */
#define HTTP_CODE_SEE_OTHER                 303     /**< The requested URI can be found at another URI location */
#define HTTP_CODE_NOT_MODIFIED              304     /**< The requested resource has changed since the last request */
#define HTTP_CODE_USE_PROXY                 305     /**< The requested resource must be accessed via the location proxy */
#define HTTP_CODE_TEMPORARY_REDIRECT        307     /**< The request should be repeated at another URI location */
#define HTTP_CODE_BAD_REQUEST               400     /**< The request is malformed */
#define HTTP_CODE_UNAUTHORIZED              401     /**< Authentication for the request has failed */
#define HTTP_CODE_PAYMENT_REQUIRED          402     /**< Reserved for future use */
#define HTTP_CODE_FORBIDDEN                 403     /**< The request was legal, but the server refuses to process */
#define HTTP_CODE_NOT_FOUND                 404     /**< The requested resource was not found */
#define HTTP_CODE_BAD_METHOD                405     /**< The request HTTP method was not supported by the resource */
#define HTTP_CODE_NOT_ACCEPTABLE            406     /**< The requested resource cannot generate the required content */
#define HTTP_CODE_REQUEST_TIMEOUT           408     /**< The server timed out waiting for the request to complete */
#define HTTP_CODE_CONFLICT                  409     /**< The request had a conflict in the request headers and URI */
#define HTTP_CODE_GONE                      410     /**< The requested resource is no longer available*/
#define HTTP_CODE_LENGTH_REQUIRED           411     /**< The request did not specify a required content length*/
#define HTTP_CODE_PRECOND_FAILED            412     /**< The server cannot satisfy one of the request preconditions */
#define HTTP_CODE_REQUEST_TOO_LARGE         413     /**< The request is too large for the server to process */
#define HTTP_CODE_REQUEST_URL_TOO_LARGE     414     /**< The request URI is too long for the server to process */
#define HTTP_CODE_UNSUPPORTED_MEDIA_TYPE    415     /**< The request media type is not supported by the server or resource */
#define HTTP_CODE_RANGE_NOT_SATISFIABLE     416     /**< The request content range does not exist for the resource */
#define HTTP_CODE_EXPECTATION_FAILED        417     /**< The server cannot satisfy the Expect header requirements */
#define HTTP_CODE_NO_RESPONSE               444     /**< The connection was closed with no response to the client */
#define HTTP_CODE_INTERNAL_SERVER_ERROR     500     /**< Server processing or configuration error. No response generated */
#define HTTP_CODE_NOT_IMPLEMENTED           501     /**< The server does not recognize the request or method */
#define HTTP_CODE_BAD_GATEWAY               502     /**< The server cannot act as a gateway for the given request */
#define HTTP_CODE_SERVICE_UNAVAILABLE       503     /**< The server is currently unavailable or overloaded */
#define HTTP_CODE_GATEWAY_TIMEOUT           504     /**< The server gateway timed out waiting for the upstream server */
#define HTTP_CODE_BAD_VERSION               505     /**< The server does not support the HTTP protocol version */
#define HTTP_CODE_INSUFFICIENT_STORAGE      507     /**< The server has insufficient storage to complete the request */

/*
    Proprietary HTTP status codes
 */
#define HTTP_CODE_START_LOCAL_ERRORS        550
#define HTTP_CODE_COMMS_ERROR               550     /**< The server had a communicationss error responding to the client */

/************************************* WebsValue ******************************/
/**
    Value types.
 */
typedef enum WebsType
{
    undefined   = 0,
    byteint     = 1,
    shortint    = 2,
    integer     = 3,
    hex         = 4,
    percent     = 5,
    octal       = 6,
    big         = 7,
    flag        = 8,
    floating    = 9,
    string      = 10,
    bytes       = 11,
    symbol      = 12,
    errmsg      = 13
} WebsType;

/**
    System native time type. This is the time in seconds.
    This may be 32 or 64 bits and may be signed or unsigned on some systems.
 */
typedef time_t WebsTime;

/**
    Value union to store primitive value types
 */
typedef struct WebsValue
{
    union
    {
        char    flag;
        char    byteint;
        short   shortint;
        char    percent;
        long    integer;
        long    hex;
        long    octal;
        long    big[2];
#if ME_FLOAT
        double  floating;
#endif
        char    *string;
        char    *bytes;
        char    *errmsg;
        void    *symbol;
    } value;
    WebsType    type;
    uint        valid       : 8;
    uint        allocated   : 8;        /* String was allocated */
} WebsValue;

/**
    The value is a numeric type
 */
#define value_numeric(t)    (t >= byteint && t <= big)

/**
    The value is a string type
 */
#define value_str(t)        (t >= string && t <= bytes)

/**
    The value is valid supported type
 */
#define value_ok(t)         (t > undefined && t <= symbol)

/**
    Allocate strings using malloc
 */
#define VALUE_ALLOCATE      0x1

/**
    Create an integer value
    @param value Integer long value
    @return Value object containing the integer
    @stability Stable
 */
PUBLIC WebsValue valueInteger(long value);

/**
    Create an string value
    @param value String long value
    @param flags Set to VALUE_ALLOCATE to store a copy of the string reference
    @return Value object containing the string
    @stability Stable
 */
PUBLIC WebsValue valueString(char *value, int flags);

/**
    Create an symbol value containing an object reference
    @param value Value reference
    @return Value object containing the symbol reference
    @stability Stable
 */
PUBLIC WebsValue valueSymbol(void *value);

/**
    Free any allocated string in a value
    @param value Value object
    @stability Stable
 */
PUBLIC void valueFree(WebsValue *value);



/******************************* Malloc Replacement ***************************/
#if ME_GOAHEAD_REPLACE_MALLOC
/**
    GoAhead allocator memory block
    Memory block classes are: 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536.
    @defgroup WebsAlloc WebsAlloc
    @stability Stable
 */
typedef struct WebsAlloc
{
    union
    {
        void    *next;                          /**< Pointer to next in q */
        int     size;                           /**< Actual requested size */
    } u;
    int         flags;                          /**< Per block allocation flags */
} WebsAlloc;

#define WEBS_DEFAULT_MEM   (64 * 1024)         /**< Default memory allocation */
#define WEBS_MAX_CLASS     13                  /**< Maximum class number + 1 */
#define WEBS_SHIFT         4                   /**< Convert size to class */
#define WEBS_ROUND         ((1 << (B_SHIFT)) - 1)
#define WEBS_MALLOCED      0x80000000          /* Block was malloced */
#define WEBS_FILL_CHAR     (0x77)              /* Fill byte for buffers */
#define WEBS_FILL_WORD     (0x77777777)        /* Fill word for buffers */

/*
    Flags. The integrity value is used as an arbitrary value to fill the flags.
 */
#define WEBS_USE_MALLOC        0x1             /**< Okay to use malloc if required */
#define WEBS_USER_BUF          0x2             /* User supplied buffer for mem */
#define WEBS_INTEGRITY         0x8124000       /* Integrity value */
#define WEBS_INTEGRITY_MASK    0xFFFF000       /* Integrity mask */
#endif /* ME_GOAHEAD_REPLACE_MALLOC */

/**
    Close the GoAhead memory allocator
    @ingroup WebsAlloc
    @stability Stable
 */
PUBLIC void wcloseAlloc();

/**
    Initialize the walloc module.
    @description The wopenAlloc function should be called the very first thing after the application starts and wclose
    should be called the last thing before exiting. If wopenAlloc is not called, it will be called on the first allocation
    with default values. "buf" points to memory to use of size "bufsize". If buf is NULL, memory is allocated using malloc.
    flags may be set to WEBS_USE_MALLOC if using malloc is okay. This routine will allocate *  an initial buffer of size
    bufsize for use by the application.
    @param buf Optional user supplied block of memory to use for allocations
    @param bufsize Size of buf
    @param flags Allocation flags. Set to WEBS_USE_MALLOC to permit the use of malloc() to grow memory.
    @return Zero if successful, otherwise -1.
    @ingroup WebsAlloc
    @stability Stable
 */
PUBLIC int wopenAlloc(void *buf, int bufsize, int flags);

/**
    Allocate a block of the requested size
    @param size Memory size required
    @return A reference to the allocated block
    @ingroup WebsAlloc
    @stability Stable
 */
PUBLIC void *walloc(ssize size);

/**
    Free an allocated block of memory
    @param blk Reference to the memory block to free.
    @ingroup WebsAlloc
    @stability Stable
 */
PUBLIC void wfree(void *blk);

/**
    Reallocate a block of memory and grow its size
    @description If the new size is larger than the existing block, a new block will be allocated and the old data
        will be copied to the new block.
    @param blk Original block reference
    @param newsize Size of the new block.
    @return Reference to the new memory block
    @ingroup WebsAlloc
    @stability Stable
 */
PUBLIC void *wrealloc(void *blk, ssize newsize);

/**
    Duplicate memory
    @param ptr Original block reference
    @param usize Size to allocate
    @return Reference to the new memory block
    @ingroup WebsAlloc
 */
PUBLIC void *wdup(cvoid *ptr, size_t usize);

typedef void (*WebsMemNotifier)(ssize size);

/**
    Define a global memory allocation notifier.
    @description The notifier is called if any memory allocation fails. It is called with the requested allocation size
        as its only parameter.
    @param cback Callback function to invoke for allocation failures.
    @ingroup WebsAlloc
    @stability Evolving
 */
PUBLIC void websSetMemNotifier(WebsMemNotifier cback);

#ifndef WEBS_SHIFT
#define WEBS_SHIFT 4
#endif

#if DEPRECATE || 1
PUBLIC ssize mtow(wchar *dest, ssize count, char *src, ssize len);
PUBLIC ssize wtom(char *dest, ssize count, wchar *src, ssize len);
PUBLIC wchar *amtow(char *src, ssize *len);
PUBLIC char  *awtom(wchar *src, ssize *len);
#endif

/******************************* Hash Table *********************************/
/**
    Hash table entry structure.
    @description The hash structure supports growable hash tables with high performance, collision resistant hashes.
    Each hash entry has a descriptor entry. This is used to manage the hash table link chains.
    @see hashCreate hashFree hashLookup hashEnter hashDelete hashWalk hashFirst hashNext
    @defgroup WebsHash WebsHash
    @stability Stable
 */
typedef struct WebsKey
{
    struct WebsKey  *forw;                  /* Pointer to next hash list */
    WebsValue       name;                   /* Name of symbol */
    WebsValue       content;                /* Value of symbol */
    int             arg;                    /* Parameter value */
    int             bucket;                 /* Bucket index */
} WebsKey;

/**
    Hash table ID returned by hashCreate
 */
typedef int WebsHash;                       /* Returned by symCreate */

/**
    Create a hash table
    @param size Minimum size of the hash index
    @return Hash table ID. Negative if the hash cannot be created.
    @ingroup WebsHash
    @stability Stable
 */
PUBLIC WebsHash hashCreate(int size);

/**
    Free a hash table
    @param id Hash table id returned by hashCreate
    @ingroup WebsHash
    @stability Stable
 */
PUBLIC void hashFree(WebsHash id);

/**
    Lookup a name in the hash table
    @param id Hash table id returned by hashCreate
    @param name Key name to search for
    @return Reference to the WebKey object storing the key and value
    @ingroup WebsHash
    @stability Stable
 */
PUBLIC WebsKey *hashLookup(WebsHash id, char *name);

/**
    Lookup a name in the hash table and return a symbol reference
    @param sd Hash table id returned by hashCreate
    @param name Key name to search for
    @return Reference to the symbole
    @ingroup WebsHash
    @stability Evolving
 */
PUBLIC void *hashLookupSymbol(WebsHash sd, char *name);

/**
    Enter a new key and value into the hash table
    @param id Hash table id returned by hashCreate
    @param name Key name to create
    @param value Key value to enter
    @param arg Optional extra argument to store with the value
    @return Reference to the WebKey object storing the key and value
    @ingroup WebsHash
    @stability Stable
 */
PUBLIC WebsKey *hashEnter(WebsHash id, char *name, WebsValue value, int arg);

/**
    Delete a key by name
    @param id Hash table id returned by hashCreate
    @param name Key name to delete
    @return Zero if the delete was successful. Otherwise -1 if the key was not found.
    @ingroup WebsHash
    @stability Stable
 */
PUBLIC int hashDelete(WebsHash id, char *name);

/**
    Start walking the hash keys by returning the first key entry in the hash
    @param id Hash table id returned by hashCreate
    @return Reference to the first WebKey object. Return null if there are no keys in the hash.
    @ingroup WebsHash
    @stability Stable
 */
PUBLIC WebsKey *hashFirst(WebsHash id);

/**
    Continue walking the hash keys by returning the next key entry in the hash
    @param id Hash table id returned by hashCreate
    @param last Reference to a WebsKey to hold the current traversal key state.
    @return Reference to the next WebKey object. Returns null if no more keys exist to be traversed.
    @ingroup WebsHash
    @stability Stable
 */
PUBLIC WebsKey *hashNext(WebsHash id, WebsKey *last);








/*********************************** Runtime **********************************/

/**
    GoAhead Web Server Runtime
    @description GoAhead provides a secure runtime environment for safe string manipulation and to
        help prevent buffer overflows and other potential security traps.
    @defgroup WebsRuntime WebsRuntime
    @see fmt wallocHandle wallocObject wfreeHandle hextoi itosbuf scaselesscmp scaselessmatch
        sclone scmp scopy sfmt sfmtv slen slower smatch sncaselesscmp sncmp sncopy stok strim supper
    @stability Stable
 */

/**
    Format a string into a static buffer.
    @description This call format a string using printf style formatting arguments. A trailing null will
        always be appended. The call returns the size of the allocated string excluding the null.
    @param buf Pointer to the buffer.
    @param maxSize Size of the buffer.
    @param format Printf style format string
    @param ... Variable arguments to format
    @return Returns the buffer.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC char *fmt(char *buf, ssize maxSize, char *format, ...);

/**
    Allocate a handle from a map
    @param map Reference to a location holding the map reference. On the first call, the map is allocated.
    @return Integer handle index. Otherwise return -1 on allocation errors.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC int wallocHandle(void *map);

/**
    Allocate an object in a halloc map
    @param map Reference to a location holding the map reference. On the first call, the map is allocated.
    @param max Reference to an integer that holds the maximum handle in the map.
    @param size Size of the object to allocate.
    @return Integer handle index. Otherwise return -1 on allocation errors.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC int wallocObject(void *map, int *max, int size);

/**
    Free a handle in the map
    @param map Reference to a location to hold the map reference.
    @param handle Handle to free in the map.
    @return Integer handle index. Otherwise return -1 on allocation errors.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC int wfreeHandle(void *map, int handle);

/**
    Convert a hex string to an integer
    @description This call converts the supplied string to an integer using base 16.
    @param str Pointer to the string to parse.
    @return Returns the integer equivalent value of the string.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC uint hextoi(char *str);

/**
    Convert an integer to a string buffer.
    @description This call converts the supplied 64 bit integer into a string formatted into the supplied buffer according
        to the specified radix.
    @param buf Pointer to the buffer that will hold the string.
    @param size Size of the buffer.
    @param value Integer value to convert
    @param radix The base radix to use when encoding the number
    @return Returns a reference to the string.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC char *itosbuf(char *buf, ssize size, int64 value, int radix);

/**
    Compare strings ignoring case. This is a safe replacement for strcasecmp. It can handle NULL args.
    @description Compare two strings ignoring case differences. This call operates similarly to strcmp.
    @param s1 First string to compare.
    @param s2 Second string to compare.
    @return Returns zero if the strings are equivalent, < 0 if s1 sorts lower than s2 in the collating sequence
        or > 0 if it sorts higher.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC int scaselesscmp(char *s1, char *s2);

/**
    Compare strings ignoring case. This is similar to scaselesscmp but it returns a boolean.
    @description Compare two strings ignoring case differences.
    @param s1 First string to compare.
    @param s2 Second string to compare.
    @return Returns true if the strings are equivalent, otherwise false.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC bool scaselessmatch(char *s1, char *s2);

/**
    Clone a string
    @description Copy a string into a newly allocated block.
    @param str Pointer to the block to duplicate.
    @return Returns a newly allocated string.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC char *sclone(char *str);

/**
    Compare strings.
    @description Compare two strings. This is a safe replacement for strcmp. It can handle null args.
    @param s1 First string to compare.
    @param s2 Second string to compare.
    @return Returns zero if the strings are identical. Return -1 if the first string is less than the second. Return 1
        if the first string is greater than the second.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC int scmp(char *s1, char *s2);

/**
    Copy a string.
    @description Safe replacement for strcpy. Copy a string and ensure the destination buffer is not overflowed.
        The call returns the length of the resultant string or an error code if it will not fit into the target
        string. This is similar to strcpy, but it will enforce a maximum size for the copied string and will
        ensure it is always terminated with a null.
    @param dest Pointer to a pointer that will hold the address of the allocated block.
    @param destMax Maximum size of the target string in characters.
    @param src String to copy
    @return Returns the number of characters in the target string.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC ssize scopy(char *dest, ssize destMax, char *src);

/*
    String trim flags
 */
#define WEBS_TRIM_START  0x1             /**< Flag for strim to trim from the start of the string */
#define WEBS_TRIM_END    0x2             /**< Flag for strim to trim from the end of the string */
#define WEBS_TRIM_BOTH   0x3             /**< Flag for strim to trim from both the start and the end of the string */

/**
    Format a string. This is a secure verion of printf that can handle null args.
    @description Format the given arguments according to the printf style format. See fmt() for a full list of the
        format specifies. This is a secure replacement for sprintf, it can handle null arguments without crashes.
    @param format Printf style format string
    @param ... Variable arguments for the format string
    @return Returns a newly allocated string
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC char *sfmt(char *format, ...);

/**
    Format a string with varargs. This is a secure verion of printf that can handle null args.
    @description Format the given arguments according to the printf style format. See fmt() for a full list of the
        format specifies. This is a secure replacement for sprintf, it can handle null arguments without crashes.
    @param format Printf style format string
    @param args Varargs argument obtained from va_start.
    @return Returns a newly allocated string
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC char *sfmtv(char *format, va_list args);

/**
    Return the length of a string.
    @description Safe replacement for strlen. This call returns the length of a string and tests if the length is
        less than a given maximum. It will return zero for NULL args.
    @param str String to measure.
    @return Returns the length of the string
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC ssize slen(char *str);

/**
    Convert a string to lower case.
    @description Convert a string to its lower case equivalent.
    @param str String to convert. This string is modified.
    @return Reference to the supplied str.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC char *slower(char *str);

/**
    Compare strings
    @description Compare two strings. This is similar to #scmp but it returns a boolean.
    @param s1 First string to compare.
    @param s2 Second string to compare.
    @return Returns true if the strings are equivalent, otherwise false.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC bool smatch(char *s1, char *s2);

/**
    Compare strings ignoring case.
    @description Compare two strings ignoring case differences for a given string length. This call operates
        similarly to strncasecmp.
    @param s1 First string to compare.
    @param s2 Second string to compare.
    @param len Length of characters to compare.
    @return Returns zero if the strings are equivalent, < 0 if s1 sorts lower than s2 in the collating sequence
        or > 0 if it sorts higher.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC int sncaselesscmp(char *s1, char *s2, ssize len);

/**
    Compare strings.
    @description Compare two strings for a given string length. This call operates similarly to strncmp.
    @param s1 First string to compare.
    @param s2 Second string to compare.
    @param len Length of characters to compare.
    @return Returns zero if the strings are equivalent, < 0 if s1 sorts lower than s2 in the collating sequence
        or > 0 if it sorts higher.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC int sncmp(char *s1, char *s2, ssize len);

/**
    Copy characters from a string.
    @description Safe replacement for strncpy. Copy bytes from a string and ensure the target string is not overflowed.
        The call returns the length of the resultant string or an error code if it will not fit into the target
        string. This is similar to strcpy, but it will enforce a maximum size for the copied string and will
        ensure it is terminated with a null.
    @param dest Pointer to a pointer that will hold the address of the allocated block.
    @param destMax Maximum size of the target string in characters.
    @param src String to copy
    @param count Maximum count of characters to copy
    @return Returns a reference to the destination if successful or NULL if the string won't fit.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC ssize sncopy(char *dest, ssize destMax, char *src, ssize count);

/*
    Test if a string is a radix 10 number.
    @description The supported format is: [(+|-)][DIGITS]
    @return true if all characters are digits or '+' or '-'
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC bool snumber(cchar *s);

/**
    Split a string at a delimiter
    @description Split a string and return parts. The string is modified.
        This routiner never returns null. If there are leading delimiters, the empty string will be returned
        and *last will be set to the portion after the delimiters.
        If str is null, an empty string will be returned.
        If there are no characters after the delimiter, then *last will be set to the empty string.
    @param str String to tokenize.
    @param delim Set of characters that are used as token separators.
    @param last Reference to the portion after the delimiters. Will return an empty string if is not trailing portion.
    @return Returns a pointer to the first part before the delimiters. If the string begins with delimiters, the empty
        string will be returned.
    @ingroup WebsRuntime
    @stability Evolving
 */
PUBLIC char *ssplit(char *str, cchar *delim, char **last);

/**
    Tokenize a string
    @description Split a string into tokens.
    @param str String to tokenize.
    @param delim String of characters to use as token separators.
    @param last Last token pointer.
    @return Returns a pointer to the next token.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC char *stok(char *str, char *delim, char **last);

/**
    Trim a string.
    @description Trim leading and trailing characters off a string.
    @param str String to trim.
    @param set String of characters to remove.
    @param where Flags to indicate trim from the start, end or both. Use WEBS_TRIM_START, WEBS_TRIM_END, WEBS_TRIM_BOTH.
    @return Returns a pointer to the trimmed string. May not equal \a str.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC char *strim(char *str, char *set, int where);

/**
    Convert a string to upper case.
    @description Convert a string to its upper case equivalent.
    @param str String to convert. This string is modified.
    @return Returns a pointer to the converted string. Will always equal str.
    @ingroup WebsRuntime
    @stability Stable
 */
PUBLIC char *supper(char *str);




/* Forward declare */
struct WebsRoute;
struct WebsUser;
struct WebsSession;
struct Webs;


/********************************** Defines ***********************************/

#define WEBS_MAX_PORT_LEN       16          /* Max digits in port number */
#define WEBS_HASH_INIT          67          /* Hash size for form table */
#define WEBS_SESSION_HASH       31          /* Hash size for session stores */
#define WEBS_SESSION_PRUNE      (60*1000)   /* Prune sessions every minute */

/*
    The license agreement stipulates that you must not change this definition.
 */
#define WEBS_NAME "Server: http"

/*
    Request flags
 */
#define WEBS_ACCEPTED           0x1         /**< TLS connection accepted */
#define WEBS_CHUNKING           0x2         /**< Currently chunking output body data */
#define WEBS_CLOSED             0x4         /**< Connection closed, ready to free */
#define WEBS_COOKIE             0x8         /**< Cookie supplied in request */
#if DEPRECATED || 1
#define WEBS_FINALIZED          0x10        /**< Output is finalized */
#endif
#define WEBS_FORM               0x20        /**< Request is a form (url encoded data) */
#define WEBS_HEADERS_CREATED    0x40        /**< Headers have been created and buffered */
#define WEBS_HTTP11             0x80        /**< Request is using HTTP/1.1 */
#define WEBS_JSON               0x100       /**< Request has a JSON payload */
#define WEBS_KEEP_ALIVE         0x200       /**< HTTP/1.1 keep alive */
#define WEBS_REROUTE            0x400       /**< Restart route matching */
#define WEBS_RESPONSE_TRACED    0x800       /**< Started tracing the response */
#define WEBS_SECURE             0x1000      /**< Connection uses SSL */
#define WEBS_UPLOAD             0x2000      /**< Multipart-mime file upload */
#define WEBS_VARS_ADDED         0x4000      /**< Query and body form vars added */
#if ME_GOAHEAD_LEGACY
#define WEBS_LOCAL              0x8000      /**< Request from local system */
#endif

/*
    Incoming chunk encoding states. Used for tx and rx chunking.
 */
#define WEBS_CHUNK_UNCHUNKED    0           /**< Data is not transfer-chunk encoded */
#define WEBS_CHUNK_START        1           /**< Start of a new chunk */
#define WEBS_CHUNK_HEADER       2           /**< Preparing tx chunk header */
#define WEBS_CHUNK_DATA         3           /**< Start of chunk data */

/*
    Webs state
 */
#define WEBS_BEGIN              0           /**< Beginning state */
#define WEBS_CONTENT            1           /**< Ready for body data */
#define WEBS_READY              2           /**< Ready to route and start handler */
#define WEBS_RUNNING            3           /**< Processing request */
#define WEBS_COMPLETE           4           /**< Request complete */

/*
    Session names
 */
#define COOKIE_SALT_CODE "The Hoely Wrold Sucks"
#define WEBS_SESSION            "S_HASH"
#define WEBS_SESSION_USERNAME   "_:USERNAME:_"  /* Username variable */
#define WEBS_SESSION_LOGINHANDLE   "_:LOGINHANDLE:_"  /* Login handle variable */
#define WEBS_SESSION_LOGINTIME  "_:LOGINTIME:_"  /* Login Time variable */
#define WEBS_SESSION_LOGINIP  "_:LOGINIP:_"  /* Login IP variable */

#define WEBS_DIGESTAUTH_TIMEOUT "AUTH_TIMEOUT"
#define WEBS_DIGESTAUTH_MAX_TIMEOUT 2*60 //2min

#define WEBS_DIGESTAUTH_ENTRY "AUTH_ENTRY"
#define WEBS_DIGESTAUTH_MAX_ENTRY_NUM 20 //一次验证通过 20次后需重新验证

/*
    WebsDone flags
 */
#define WEBS_CODE_MASK      0xFFFF      /**< Mask valid status codes */
#define WEBS_CLOSE          0x20000     /**< Close connection */
#define WEBS_NOLOG          0x40000     /**< Don't write error to log */

/**
    Callback for write I/O events
 */
typedef void (*WebsWriteProc)(struct Webs *wp);

/**
    GoAhead request structure. This is a per-socket connection structure.
    @defgroup Webs Webs
 */
typedef struct Webs
{
    // WebsBuf         rxbuf;              /**< Raw receive buffer */
    // WebsBuf         input;              /**< Receive buffer after de-chunking */
    //WebsBuf         output;             /**< Transmit buffer after chunking */
    //WebsBuf         chunkbuf;           /**< Pre-chunking data buffer */
    //WebsBuf         *txbuf;
    WebsTime        since;              /**< Parsed if-modified-since time */
    WebsTime        timestamp;          /**< Last transaction with browser */
    WebsHash        vars;               /**< fCGI standard variables */
    int             timeout;            /**< Timeout handle */
    char            ipaddr[ME_MAX_IP];  /**< Connecting ipaddress */
    char            ifaddr[ME_MAX_IP];  /**< Local interface ipaddress */

    int             rxChunkState;       /**< Rx chunk encoding state */
    ssize           rxChunkSize;        /**< Rx chunk size */
    char            *rxEndp;            /**< Pointer to end of raw data in input beyond endp */
    ssize           lastRead;           /**< Number of bytes last read from the socket */
    bool            eof;                /**< If at the end of the request content */

    char            txChunkPrefix[16];  /**< Transmit chunk prefix */
    char            *txChunkPrefixNext; /**< Current I/O pos in txChunkPrefix */
    ssize           txChunkPrefixLen;   /**< Length of prefix */
    ssize           txChunkLen;         /**< Length of the chunk */
    int             txChunkState;       /**< Transmit chunk state */

    char            *authDetails;       /**< Http header auth details */
    char            *authResponse;      /**< Outgoing auth header */
    char            *authType;          /**< Authorization type (Basic/DAA) */
    char            *contentType;       /**< Body content type */
    char            *cookie;            /**< Request cookie string */
    char            *decodedQuery;      /**< Decoded request query */
    char            *digest;            /**< Password digest */
    char            *ext;               /**< Path extension */
    char            *filename;          /**< Document path name */
    char            *host;              /**< Requested host */
    char            *method;            /**< HTTP request method */
    char            *password;          /**< Authorization password */
    char            *path;              /**< Path name without query. This is decoded. */
    char            *pathlast;          /**< Last part of Path name without query. This is decoded. */
    char            *protoVersion;      /**< Protocol version (HTTP/1.1)*/
    char            *protocol;          /**< Protocol scheme (normally http|https) */
    char            *putname;           /**< PUT temporary filename */
    char            *query;             /**< Request query. This is decoded. */
    char            *realm;             /**< Realm field supplied in auth header */
    char            *referrer;          /**< The referring page */
    char            *responseCookie;    /**< Outgoing cookie */
    char            *responseCookieHash;/**hash check vals**/
    char            *responseX_Csrf_TokenHash;/**hash check ajax request**/
    char            *url;               /**< Full request url. This is not decoded. */
    char            *queryString;               /**< QUERY_STRING in CGI. This is not decoded. */
    char            *userAgent;         /**< User agent (browser) */
    char            *username;          /**< Authorization username */
    int             sid;                /**< Socket id (handler) */
    int             listenSid;          /**< Listen Socket id */
    int             port;               /**< Request port number */
    int             state;              /**< Current state */
    int             flags;              /**< Current flags -- see above */
    int             code;               /**< Response status code */
    int             routeCount;         /**< Route count limiter */
    ssize           rxLen;              /**< Rx content length */
    ssize           rxRemaining;        /**< Remaining content to read from client */
    ssize           txLen;              /**< Tx content length header value */
    int             wid;                /**< Index into webs */
#if ME_GOAHEAD_CGI
    char            *cgiStdin;          /**< Filename for CGI program input */
    int             cgifd;              /**< File handle for CGI program input */
#endif
#if !ME_ROM
    int             putfd;              /**< File handle to write PUT data */
#endif
    int             docfd;              /**< File descriptor for document being served */
    ssize           written;            /**< Bytes actually transferred */
    ssize           putLen;             /**< Bytes read by a PUT request */

    int             finalized: 1;          /**< Request has been completed */
    int             error: 1;              /**< Request has an error */
    int             connError: 1;          /**< Request has a connection error */
    char*           session_id;
    struct WebsSession *session;        /**< Session record */
    struct WebsRoute *route;            /**< Request route */
    struct WebsUser *user;              /**< User auth record */
    WebsWriteProc   writeData;          /**< Handler write I/O event callback. Used by fileHandler */
    int             encoded;            /**< True if the password is MD5(username:realm:password) */
#if ME_GOAHEAD_DIGEST
    char            *cnonce;            /**< check nonce */
    char            *digestUri;         /**< URI found in digest header */
    char            *nonce;             /**< opaque-to-client string sent by server */
    char            *nc;                /**< nonce count */
    char            *opaque;            /**< opaque value passed from server */
    char            *qop;               /**< quality operator */
#endif
#if ME_GOAHEAD_UPLOAD
    int             upfd;               /**< Upload file handle */
    WebsHash        files;              /**< Uploaded files */
    char            *boundary;          /**< Mime boundary (static) */
    ssize           boundaryLen;        /**< Boundary length */
    int             uploadState;        /**< Current file upload state */
    WebsUpload      *currentFile;       /**< Current file context */
    char            *clientFilename;    /**< Current file filename */
    char            *uploadTmp;         /**< Current temp filename for upload data */
    char            *uploadVar;         /**< Current upload form variable name */
#endif
    void            *ssl;               /**< SSL context */


    FCGX_Request* req;
    int  fixIEMode;//for IE kit browsers,they treat 4xx http code as Errors which make our Digest Auth failed! use this flag to open fix mode!

    //WSSE UserToken auth
    char      *wsse_created;
    char      *wsse_nonce;
    char      *wsse_passwordDigest;
    char      *upload_filename[6];
    char      *uploadType;
    // 线程序号
    int       thread_index;
} Webs;


typedef Webs* webs_t;




/**
    GoAhead handler service callback
    @param wp Webs request object
    @return True if the handler serviced the request
    @ingroup Webs
    @stability Stable
 */
typedef bool (*WebsHandlerProc)(Webs *wp);

/**
    GoAhead handler close to release memory prior to shutdown.
    @description This callback is invoked when GoAhead is shutting down.
    @ingroup Webs
    @stability Stable
 */
typedef void (*WebsHandlerClose)();

/**
    GoAhead handler object
    @ingroup Webs
    @stability Stable
 */
typedef struct WebsHandler
{
    char                *name;              /**< Handler name */
    WebsHandlerProc     match;              /**< Handler match callback */
    WebsHandlerProc     service;            /**< Handler service callback */
    WebsHandlerClose    close;              /**< Handler close callback  */
    int                 flags;              /**< Handler control flags */
} WebsHandler;

/**
    Action callback
    @param wp Webs request object
    @ingroup Webs
    @stability Stable
 */
typedef void (*WebsAction)(Webs *wp);


//typedef void (*WebsProc)(Webs *wp, char *path, char *query);


/**
    Error code list
    @ingroup Webs
    @stability Stable
 */
typedef struct WebsError
{
    int     code;                           /**< HTTP error code */
    char    *msg;                           /**< HTTP error message */
} WebsError;

/**
    Mime type list
    @ingroup Webs
    @stability Stable
 */
typedef struct WebsMime
{
    char    *type;                          /**< Mime type */
    char    *ext;                           /**< File extension */
} WebsMime;





#define WEBS_DECODE_TOKEQ 1                 /**< Decode base 64 blocks up to a NULL or equals */


/**
    Open the action handler
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websActionOpen();

/**
    Allocate a new Webs object
    @param sid Socket ID handle for the newly accepted socket
    @return The webs[] handle index for the allocated Webs object
    @ingroup Webs
    @stability Stable
 */
PUBLIC int websAlloc(int sid);

/**
    Cancel the request timeout.
    @description Handlers may choose to manually manage the request timeout. This routine will disable the
        centralized management of the timeout for this request.
    @param wp Webs request object
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websCancelTimeout(Webs *wp);



/**
    Close the core GoAhead web server module
    @description Invoked when GoAhead is shutting down.
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websClose();

/**
    Close an open file
    @param fd Open file handle returned by websOpenFile
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websCloseFile(int fd);

/**
    Compare a request variable
    @param wp Webs request object
    @param var Variable name
    @param value Value to compare with
    @return True if the value matches. Otherwise return 0
    @ingroup Webs
    @stability Stable
 */
PUBLIC int websCompareVar(Webs *wp, char *var, char *value);

/**
    Consume input from the request input buffer.
    @description This is called by handlers when consuming data from the request input buffer.
        This call updates the input service pointers and compacts the input buffer if required.
    @param wp Webs request object
    @param nbytes Number of bytes the handler has consumed from the input buffer.
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websConsumeInput(Webs *wp, ssize nbytes);

/**
    Decode the string using base-64 encoding
    @description This modifies the original string
    @param str String to decode
    @return The original string.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websDecode64(char *str);

/**
    Decode a block using base-46 encoding
    @param str String to decode. The string must be null terminated.
    @param len Reference to an integer holding the length of the decoded string.
    @param flags Reserved.
    @return The original string.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websDecode64Block(char *str, ssize *len, int flags);

/**
    Decode a URL expanding %NN encoding
    @description Supports insitu decoding. i.e. Input and output buffers may be the same.
    @param decoded Buffer to hold the decoded URL
    @param input Input URL or buffer to decode
    @param len Length of the decoded buffer.
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websDecodeUrl(char *decoded, char *input, ssize len);

/**
    Define a request handler
    @param name Name of the handler
    @param match Handler callback match procedure. Invoked to match the request with the handler.
        The handler should return true to accept the request.
    @param service Handler callback service procedure. Invoked to service each request.
    @param close Handler callback close procedure. Called when GoAhead is shutting down.
    @param flags Set to WEBS_LEGACY_HANDLER to support the legacy handler API calling sequence.
    @return Zero if successful, otherwise -1.
    @ingroup Webs
    @stability Stable
 */
PUBLIC int websDefineHandler(char *name, WebsHandlerProc match, WebsHandlerProc service, WebsHandlerClose close, int flags);

/**
    Complete a request.
    @description A handler should call websDone() to complete the request.
    @param wp Webs request object
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websDone(Webs *wp);

/**
    Encode a string using base-64 encoding
    @description The string is encoded insitu.
    @param str String to encode
    @return The original string.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websEncode64(char *str);

/**
    Encode a block using base-64 encoding
    @description The string is encoded insitu.
    @param str String to encode.
    @param len Length of string to encode
    @return The original string.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websEncode64Block(char *str, ssize len);

/**
    Escape unsafe characters in a string
    @param str String to escape
    @return An allocated block containing the escaped string. Caller must free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websEscapeHtml(char *str);

/**
    Complete a request with an error response
    @param wp Webs request object
    @param code HTTP status code
    @param fmt Message printf style format
    @param ... Format args
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websError(Webs *wp, int code, char *fmt, ...);

/**
    Get a message for a HTTP status code
    @param code HTTP status code
    @return Http status message
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websErrorMsg(int code);





/**
    Free the webs request object.
    @description Callers should call websDone to complete requests prior to invoking websFree.
    @param wp Webs request object
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websFree(Webs *wp);

/**
    Get the background execution flag
    @description If GoAhead is invoked with --background, it will run as a daemon in the background.
    @return True if GoAhead is running in the background.
    @ingroup Webs
    @stability Stable
 */
PUBLIC int websGetBackground();


/**
    Get the request cookie if supplied
    @param wp Webs request object
    @return Cookie string if defined, otherwise null.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetCookie(Webs *wp);



/**
    Get the debug flag
    @description If GoAhead is invoked with --debugger, the debug flag will be set to true
    @return True if GoAhead is running in debug mode.
    @ingroup Webs
    @stability Stable
 */
PUBLIC int websGetDebug();






/**
    Get the request URI extension
    @param wp Webs request object
    @return The URI filename extension component. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetExt(Webs *wp);

/**
    Get the request filename
    @description The URI is mapped to a filename by decoding and prepending with the request directory.
    @param wp Webs request object
    @return Filename string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetFilename(Webs *wp);

/**
    Get the request host
    @description The request host is set to the Host HTTP header value if it is present. Otherwise it is set to
        the request URI hostname.
    @param wp Webs request object
    @return Host string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetHost(Webs *wp);

/**
    Get the request interface address
    @param wp Webs request object
    @return Network interface string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetIfaddr(Webs *wp);

/**
    Get the default index document name
    @description The default index is "index.html" and can be updated via websSetIndex.
    @return Index name string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetIndex();

/**
    Get the request method
    @param wp Webs request object
    @return HTTP method string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetMethod(Webs *wp);

/**
    Get the request password
    @description The request password may be encoded depending on the authentication scheme.
        See wp->encoded to test if it is encoded.
    @param wp Webs request object
    @return Password string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetPassword(Webs *wp);

/**
    Get the request path
    @description The URI path component excludes the http protocol, hostname, port, reference and query components.
    It always beings with "/".
    @param wp Webs request object
    @return Request path string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetPath(Webs *wp);

/**
    Get the request TCP/IP port
    @param wp Webs request object
    @return TCP/IP Port integer
    @ingroup Webs
    @stability Stable
 */
PUBLIC int websGetPort(Webs *wp);

/**
    Get the request HTTP protocol
    @description This will be set to either "http" or "https"
    @param wp Webs request object
    @return Protocol string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetProtocol(Webs *wp);

/**
    Get the request query component
    @param wp Webs request object
    @return Request query string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetQuery(Webs *wp);

/**
    Get the server host name
    @return Host name string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetServer();

/**
    Get the server host name with port number.
    @return Host name string with port number. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetServerUrl();

/**
    Get the server IP address
    @return Server IP address string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetServerAddress();

/**
    Get the server IP address with port number
    @return Server IP:PORT address string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetServerAddressUrl();

/**
    Get the request URI
    @description This returns the request URI. This may be modified if the request is rewritten via websRewrite
    @param wp Webs request object
    @return URI string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetUrl(Webs *wp);

/**
    Get the client User-Agent HTTP header
    @param wp Webs request object
    @return User-Agent string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetUserAgent(Webs *wp);

/**
    Get the request username
    @description If the request is authenticated, this call returns the username supplied during authentication.
    @param wp Webs request object
    @return Username string if defined, otherwise null. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetUsername(Webs *wp);

/**
    Get a request variable
    @description Request variables are defined for HTTP headers of the form HTTP_*.
        Some request handlers also define their own variables. For example: CGI environment variables.
    @param wp Webs request object
    @param name Variable name
    @param defaultValue Default value to return if the variable is not defined
    @return Variable value string. Caller should not free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websGetVar(Webs *wp, char *name, char *defaultValue);



/**
    Get an MD5 digest of a string
    @param str String to analyze.
    @return Allocated MD5 checksum. Caller should free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websMD5(char *str);

/**
    Get an MD5 digest of a block and optionally prepend a prefix.
    @param buf Block to analyze
    @param length Length of block
    @param prefix Optional prefix to prepend to the MD5 sum.
    @return Allocated MD5 checksum. Caller should free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websMD5Block(char *buf, ssize length, char *prefix);

/**
    Normalize a URI path
    @description This removes "./", "../" and redundant separators.
    @param path URI path to normalize
    @return An allocated normalized URI path. Caller must free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websNormalizeUriPath(char *path);

/**
    Take not of the request activity and mark the time.
    @description This is used to defer the request timeout whenever there is request I/O activity.
    @param wp Webs request object
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websNoteRequestActivity(Webs *wp);

/**
    Close the runtime code.
    @description Called from websClose
    @ingroup Webs
    @internal
 */
PUBLIC void websRuntimeClose();


/**
    Open the web server
    @description This initializes the web server and defines the documents directory.
    @param documents Optional web documents directory. If set to null, the build time ME_GOAHEAD_DOCUMENTS value
        is used for the documents directory.
    @param routes Optional filename for a route configuration file to load. Additional route or
        authentication configuration files can be loaded via websLoad.
    @param routes Webs request object
    @return Zero if successful, otherwise -1.
    @ingroup Webs
    @stability Stable
 */
PUBLIC int websOpen(/*char *documents, char *routes*/);

/**
    Close the O/S dependant code.
    @description Called from websClose
    @ingroup Webs
    @internal
 */
PUBLIC void websOsClose();

/**
    Open the O/S dependant code.
    @description Called from websOpen
    @return Zero if successful, otherwise -1.
    @ingroup Webs
    @internal
 */
PUBLIC int websOsOpen();



/**
    Define an action callback for use with the action handler.
    @description The action handler binds a C function to a URI under "/action".
    @param name URI path suffix. This suffix is added to "/action" to form the bound URI path.
    @param fun Callback function. The signature is void (*WebsAction)(Webs *wp);
    @return Zero if successful, otherwise -1.
    @ingroup Webs
    @stability Stable
 */
PUBLIC int websDefineAction(cchar *name, void *fun);



/**
    Redirect the client to a new URL.
    @description This creates a response to the client with a Location header directing the client to a new location.
        The response uses a 302 HTTP status code.
    @param wp Webs request object
    @param url URL to direct the client to.
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websRedirect(Webs *wp, char *url);

/**
    Redirect the client to a new URI
    @description The routing configuration file can define redirection routes for various HTTP status codes.
        This routine will utilize the appropriate route redirection based on the request route and specified status code.
    @param wp Webs request object
    @param status HTTP status code to use in selecting the route redirection.
    @return Zero if successful, otherwise -1.
    @ingroup Webs
    @stability Stable
 */
PUBLIC int websRedirectByStatus(Webs *wp, int status);

/**
    Create and send a request response
    @description This creates a response for the current request using the specified HTTP status code and
        the supplied message.
    @param wp Webs request object
    @param status HTTP status code.
    @param msg Response message body
    @return Zero if successful, otherwise -1.
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websResponse(Webs *wp, int status, char *msg);

/**
    Rewrite a request
    @description Handlers may choose to not process a request but rather rewrite requests and then reroute.
    @param wp Webs request object
    @param url New request URL.
    @return Zero if successful, otherwise -1.
    @ingroup Webs
    @stability Stable
 */
PUBLIC int websRewriteRequest(Webs *wp, char *url);









/*
    Flags for websSetCookie
 */
#define WEBS_COOKIE_SECURE   0x1         /**< Flag for websSetCookie for secure cookies (https only) */
#define WEBS_COOKIE_HTTP     0x2         /**< Flag for websSetCookie for http cookies (http only) */

/**
    Define a cookie to include in the response
    @param wp Webs request object
    @param name Cookie name
    @param value Cookie value
    @param path URI path prefix applicable for this cookie
    @param domain Domain applicable for this cookie
    @param lifespan Cookie lifespan in secons
    @param flags Set to WEBS_COOKIE_SECURE for https only. Set to WEBS_COOKIE_HTTP for http only.
        Otherwise the cookie applies to both http and https requests.
    @return Zero if successful, otherwise -1.
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websSetCookie(Webs *wp, char *name, char *value, char *path, char *domain, int lifespan, int flags);

/**
    Set the debug processing flag
    @param on Value to set the debug flag to.
    @ingroup Webs
    @internal
 */
PUBLIC void websSetDebug(int on);





/**
    Define the host name for the server
    @param host String host name
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websSetHost(char *host);

/**
    Define the host IP address
    @param ipaddr Host IP address
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websSetIpAddr(char *ipaddr);

/**
    Create and send a request response
    @description This creates a response for the current request using the specified HTTP status code and
        the supplied message.
    @param filename Web document name to use as the index. This should not contain any directory components.
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websSetIndex(char *filename);

/**
    Create request variables for query string data
    @param wp Webs request object
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websSetQueryVars(Webs *wp);

/**
    Set the response HTTP status code
    @param wp Webs request object
    @param status HTTP status code
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websSetStatus(Webs *wp, int status);

/**
    Set the response body content length
    @param wp Webs request object
    @param length Length value to use
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websSetTxLength(Webs *wp, ssize length);

/**
    Set a request variable to a formatted string value
    @description Request variables are defined for HTTP headers of the form HTTP_*.
        Some request handlers also define their own variables. For example: CGI environment variables.
    @param wp Webs request object
    @param name Variable name to set
    @param fmt Value format string
    @param ... Args to format
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websSetVarFmt(Webs *wp, char *name, char *fmt, ...);


/**
    Open the date/time parsing module
    @return Zero if successful, otherwise -1.
    @ingroup Webs
    @stability Evolving
 */
PUBLIC int websTimeOpen();

/**
    Close the date/time parsing module
    @ingroup Webs
    @stability Evolving
*/
PUBLIC void websTimeClose();

/**
    Parse a URL into its components
    @param url URL to parse
    @param buf Buffer to hold storage for various parsed components. Caller must free. NOTE: the parsed components may
        point to locations in this buffer.
    @param protocol Parsed URL protocol component
    @param host Parsed hostname
    @param port Parsed URL port
    @param path Parsed URL path component
    @param ext Parsed URL extension
    @param reference Parsed URL reference portion (\#reference)
    @param query Parsed URL query component
    @return Zero if successful, otherwise -1.
    @ingroup Webs
    @stability Evolving
 */
PUBLIC int websUrlParse(char *url, char **buf, char **protocol, char **host, char **port, char **path, char **ext,
                        char **reference, char **query);

/**
    Test if a webs object is valid
    @description After calling websDone, the websFree routine will have been called and the memory for the webs object
        will be released. Call websValid to test a Webs object for validity.
    @param wp Webs request object
    @return True if the webs object is still valid and the request has not been completed.
    @ingroup Webs
    @stability Stable
 */
PUBLIC bool websValid(Webs *wp);

/**
    Validate a URI path as expected in a HTTP request line
    @description This expects a URI beginning with "/" and containing only valid URI characters.
    The URI is decoded, and normalized removing "../" and "." segments.
    The URI must begin with a "/" both before and after decoding and normalization.
    @param uri URI to validate.
    @return A validated, normalized URI path. Caller must free.
    @ingroup Webs
    @stability Stable
 */
PUBLIC char *websValidateUriPath(char *uri);

/**
    Test if a URI is using only valid characters
    Note this does not test if the URI is fully legal. Some components of the URI have restricted character sets
    that this routine does not test. This tests if the URI has only characters valid to use in a URI before decoding.
    i.e. It will permit %NN encodings. The set of valid characters is:
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;=%"
    @param uri Uri to test
    @return True if the URI string is comprised of legal URI characters.
    @ingroup Webs
    @stability Evolving
  */
PUBLIC bool websValidUriChars(char *uri);

/**
    Write a set of standard response headers
    @param wp Webs request object
    @param contentLength Value for the Content-Length header which describes the length of the response body
    @param redirect Value for the Location header which redirects the client to a new URL.
    @ingroup Webs
    @see websSetStatus
    @stability Stable
 */
PUBLIC void websWriteHeaders(Webs *wp, ssize contentLength, char *redirect);

/**
    Signify the end of the response headers
    @description This call concludes the response headers and writes a blank line to the response.
    @param wp Webs request object
    @ingroup Webs
    @stability Stable
 */
PUBLIC void websWriteEndHeaders(Webs *wp);

/**
    Write a response header
    @description This routine writes a response header. It should be invoked after calling websWriteHeaders
        to write the standard headers and before websWriteEndHeaders.
        This routine differs from websWrite in that it traces header values to the log.
    @param wp Webs request object
    @param key Header key value
    @param fmt Header value format string.
    @param ... Arguments to the format string.
    @return Zero if successful, otherwise -1.
    @ingroup Webs
    @stability Stable
 */
PUBLIC int websWriteHeader(Webs *wp, char *key, char *fmt, ...);

/**
    Write data to the response
    @description The data is buffered and will be sent to the client when the buffer is full or websFlush is
        called.
    @param wp Webs request object
    @param fmt Printf style format string.
    @param ... Arguments to the format string.
    @return Count of bytes written
    @ingroup Webs
    @stability Stable
 */
PUBLIC ssize websWrite(Webs *wp, char *fmt, ...);


/**
    Write a block of data to the response
    @description The data is buffered and will be sent to the client when the buffer is full or websFlush is
        called. This routine will never return "short", it will always write all the data unless there are errors.
    @param wp Webs request object
    @param buf Buffer of data to write
    @param size Length of buf
    @return Count of bytes written or -1. This will always equal size if there are no errors.
    @ingroup Webs
    @stability Stable
 */
PUBLIC ssize websWriteBlock(Webs *wp, char *buf, ssize size);







/************************************** Crypto ********************************/

/**
    Get some random data
    @param buf Reference to a buffer to hold the random data
    @param length Size of the buffer
    @param block Set to true if it is acceptable to block while accumulating entropy sufficient to provide good
        random data. Setting to false will cause this API to not block and may return random data of a lower quality.
    @ingroup Crypto
    @stability Prototype.
  */
PUBLIC int websGetRandomBytes(char *buf, ssize length, bool block);

/**
    Encrypt a password using the Blowfish algorithm
    @param password User's password to encrypt
    @param salt Salt text to add to password. Helps to make each user's password unique.
    @param rounds Number of times to encrypt. More times, makes the routine slower and passwords harder to crack.
    @return The encrypted password.
    @ingroup Crypto
    @stability Prototype
 */
PUBLIC char *websCryptPassword(char *password, char *salt, int rounds);

/**
    Make salt for adding to a password.
    @param size Size in bytes of the salt text.
    @return The random salt text.
    @ingroup Crypto
    @stability Prototype
 */
PUBLIC char *websMakeSalt(ssize size);

/**
    Make a password hash for a plain-text password using the Blowfish algorithm.
    @param password User's password to encrypt
    @param saltLength Length of salt text to add to password. Helps to make each user's password unique.
    @param rounds Number of times to encrypt. More times, makes the routine slower and passwords harder to crack.
    @return The encrypted password.
    @ingroup Crypto
    @stability Prototype
 */
PUBLIC char *websMakePassword(char *password, int saltLength, int rounds);

/**
    Check a plain-text password against the defined hashed password.
    @param plainTextPassword User's plain-text-password to check
    @param passwordHash Required password in hashed format previously computed by websMakePassword.
    @return True if the password is correct.
    @ingroup Crypto
    @stability Prototype
 */
PUBLIC bool websCheckPassword(char *plainTextPassword, char *passwordHash);

/**
    Get a password from the terminal console
    @param prompt Text prompt to display before reading the password
    @return The entered password.
    @ingroup Crypto
    @stability Prototype
 */
PUBLIC char *websReadPassword(char *prompt);






/*************************************** Route *********************************/
/**
    Callback to prompt the user for their password
    @param wp Webs request object
    @ingroup Webs
    @stability Stable
 */
typedef void (*WebsAskLogin)(Webs *wp);

/**
    Callback to verify the username and password
    @param wp Webs request object
    @return True if the password is verified
    @ingroup Webs
    @stability Stable
 */
typedef bool (*WebsVerify)(Webs *wp);

/**
    Callback to parse authentication details submitted with the web request
    @param wp Webs request object
    @return True if the details can be parsed
    @ingroup Webs
    @stability Stable
 */
typedef bool (*WebsParseAuth)(Webs *wp);

/**
    Request route structure
    @defgroup WebsRoute WebsRoute
 */
typedef struct WebsRoute
{
    char            *prefix;                /**< Route path prefix */
    ssize           prefixLen;              /**< Prefix length */
    char            *dir;                   /**< Filesystem base directory for route documents */
    char            *protocol;              /**< HTTP protocol to use for this route */
    char            *authType;              /**< Authentication type */
    WebsHandler     *handler;               /**< Request handler to service requests */
    WebsHash        abilities;              /**< Required user abilities */
    WebsHash        extensions;             /**< Permissible URI extensions */
    WebsHash        redirects;              /**< Response redirections */
    WebsHash        methods;                /**< Supported HTTP methods */
    WebsAskLogin    askLogin;               /**< Route path prefix */
    WebsParseAuth   parseAuth;              /**< Parse authentication details callback*/
    WebsVerify      verify;                 /**< Verify password callback */
    int             flags;                  /**< Route control flags */
} WebsRoute;

/**
    Add a route to the routing tables
    @param uri Matching URI prefix
    @param handler Request handler to service routed requests
    @param pos Position in the list of routes. Zero inserts at the front of the list. A value of -1 will append to the
        end of the list.
    @return A route object
    @ingroup WebsRoute
    @stability Stable
 */
PUBLIC WebsRoute *websAddRoute(char *uri, char *handler, int pos);

/**
    Close the route module
    @ingroup WebsRoute
    @stability Stable
 */
PUBLIC void websCloseRoute();



/**
    Open the routing module
    @ingroup WebsRoute
    @stability Stable
 */
PUBLIC int websOpenRoute();

/**
    Remove a route from the routing tables
    @param uri Matching URI prefix
    @return Zero if successful, otherwise -1.
    @ingroup WebsRoute
    @stability Stable
 */
PUBLIC int websRemoveRoute(char *uri);

/**
    Route a request
    @description This routine will select a matching route and will invoke the selected route handler to service
        the request. In the process, authentication and request rewriting may take place.
        This routine is called internally by the request pipeline.
    @param wp Webs request object
    @ingroup WebsRoute
    @stability Stable
 */
PUBLIC void websRouteRequest(Webs *wp,char*);

void webs_route_request(Webs *wp);

/**
    Configure a route by adding matching criteria
    @param route Route to modify
    @param dir Set the route documents directory filename
    @param protocol Set the matching HTTP protocol (http or https)
    @param methods Hash of permissible HTTP methods. (GET, HEAD, POST, PUT)
    @param extensions Hash of permissible URI filename extensions.
    @param abilities Required user abilities. The user must be authenticated.
    @param abilities Required user abilities. If abilities are required, the user must be authenticated.
    @param redirects Set of applicable response redirections when completing the request.
    @return Zero if successful, otherwise -1.
    @ingroup WebsRoute
    @stability Evolving
 */
PUBLIC int websSetRouteMatch(WebsRoute *route, char *dir, char *protocol, WebsHash methods, WebsHash extensions,
                             WebsHash abilities, WebsHash redirects);

/**
    Set route authentication scheme
    @param route Route to modify
    @param authType Set to "basic", "digest" or "form".
    @return Zero if successful, otherwise -1.
    @ingroup WebsRoute
    @stability Stable
 */
PUBLIC int websSetRouteAuth(WebsRoute *route, char *authType);


/*************************************** Auth **********************************/
#if ME_GOAHEAD_AUTH

#define WEBS_USIZE          128              /* Size of realm:username */

/**
    GoAhead Authentication
    @defgroup WebsAuth WebsAuth
 */
/**
    User definition structure
    @ingroup WebsAuth
    @stability Stable
 */
typedef struct WebsUser
{
    char    *name;                          /**< User name */
    char    *password;                      /**< User password (encrypted) */
    char    *pwd;                           /* User password (not encrypted) */
    char    *roles;                         /**< User roles */
    WebsHash abilities;                     /**< Resolved user abilities */
} WebsUser;




/**
    Add a user
    @description The user is added to the list of users
    @param username User name
    @param password User password (not encrypted)
    @param password User password (encrypted)
    @param roles Space separated list of roles. This may also contain abilities.
    @return User object.
    @ingroup WebsAuth
    @stability Stable
 */
PUBLIC WebsUser *websAddUser(char *username, char *password, char *encrypted_password);

/**
    Authenticate a user
    @description The user is authenticated if required by the selected request route.
    @param wp Webs request object
    @return True if the route does not require authentication or the user is authenticated successfully.
    @ingroup WebsAuth
    @stability Stable
 */
PUBLIC bool websAuthenticate(Webs *wp,char*);


/**
    Close the authentication module
    @ingroup WebsAuth
    @stability Stable
 */
PUBLIC void websCloseAuth();



/**
    Set the password store verify callback
    @return verify WebsVerify callback function
    @ingroup WebsAuth
    @stability Stable
 */
PUBLIC WebsVerify websGetPasswordStoreVerify();



/**
    Get the users hash
    @return The users hash object
    @ingroup WebsAuth
    @stability Stable
 */
PUBLIC WebsHash websGetUsers();




/**
    Lookup if a user exists
    @param username User name to search for
    @return User object or null if the user cannot be found
    @ingroup WebsAuth
    @stability Stable
 */
PUBLIC WebsUser *websLookupUser(char *username);



/**
    Remove a user from the system
    @param name User name
    @return Zero if successful, otherwise -1
    @ingroup WebsAuth
    @stability Stable
 */
PUBLIC int websRemoveUser(char *name);

/**
    Open the authentication module
    @param minimal Reserved. Set to zero.
    @return True if the user has the required ability.
    @ingroup WebsAuth
    @stability Stable
 */
PUBLIC int websOpenAuth(int minimal);

/**
    Set the password store verify callback
    @param verify WebsVerify callback function
    @ingroup WebsAuth
    @stability Stable
 */
PUBLIC void websSetPasswordStoreVerify(WebsVerify verify);

/**
    Set a password for the user
    @param username User name
    @param password Null terminated password string
    @return Zero if successful, otherwise -1.
    @ingroup WebsAuth
    @stability Stable
 */
PUBLIC int websSetUserPassword(char *username, char *password, char *encrypted_password);





/**
    User password verification routine from auth.txt
    @param wp Webs request object
    @return True if the user password verifies.
    @ingroup WebsAuth
    @stability Stable
 */
PUBLIC bool websVerifyPasswordFromFile(Webs *wp);

#endif /* ME_GOAHEAD_AUTH */




/************************************** Sessions *******************************/
/**
    Session state storage
    @defgroup WebsSession WebsSession
 */
typedef struct WebsSession
{
    char            *id;                    /**< Session ID key */
    int             lifespan;               /**< Session inactivity timeout (msecs) */
    WebsTime        expires;                /**< When the session expires */
    WebsHash        cache;                  /**< Cache of session variables */
} WebsSession;

/**
    Test if a user possesses the required ability
    @param wp Webs request object
    @param id Session ID to use. Set to null to allocate a new session ID.
    @param lifespan Lifespan of the session in seconds.
    @return Allocated session object
    @ingroup WebsSession
    @stability Stable
 */
PUBLIC WebsSession *websAllocSession(Webs *wp, char *id, int lifespan);

/**
    Get the session ID
    @param wp Webs request object
    @return The session ID if session state storage is defined for this request.
    @ingroup WebsSession
    @stability Stable
 */
PUBLIC char *websGetValueFromCookie(Webs *wp,char* name);

PUBLIC int websSetNewSession(Webs *wp);
PUBLIC WebsSession* websGetSessionBySSID(Webs *wp, char *key);


/**
    Get a session variable
    @param wp Webs request object
    @param name Session variable name
    @param defaultValue Default value to return if the variable does not exist
    @return Session variable value or default value if it does not exist
    @ingroup WebsSession
    @stability Stable
 */
PUBLIC WebsValue *websGetSessionVar(Webs *wp, char *name);

/**
    Remove a session variable
    @param wp Webs request object
    @param name Session variable name
    @ingroup WebsSession
    @stability Stable
 */
PUBLIC void websRemoveSessionVar(Webs *wp, char *name);

/**
    Set a session variable name value
    @param wp Webs request object
    @param name Session variable name
    @param value Value to set the variable to
    @return Zero if successful, otherwise -1
    @ingroup WebsSession
    @stability Stable
 */
PUBLIC int websSetSessionVar(Webs *wp, char *name, WebsValue value);

/************************************ Legacy **********************************/
/*
    Legacy mappings for pre GoAhead 3.X applications
    This is a list of the name changes from GoAhead 2.X to GoAhead 3.x
    To maximize forward compatibility, It is best to not use ME_GOAHEAD_LEGACY except as
    a transitional compilation aid.
 */



/*

    Nginx Server Functions

*/
#define NGX_ROOT_PATH "/root/nginx"
#define NGX_ROOT_PATH2 "/update/nginx"



#define NGX_HTML NGX_ROOT_PATH "/html"
#define NGX_PLUGIN_HTML NGX_ROOT_PATH "/html/plugin-web"

#define NGX_SSL NGX_ROOT_PATH "/ssl"
#define NGX_CONF_FILE NGX_ROOT_PATH "/conf/nginx.conf"
#define NGX_CONF_FILE2 NGX_ROOT_PATH2 "/conf/nginx.conf"
#define NGX_CONF_TMPFILE NGX_ROOT_PATH "/conf/nginx_tmp.conf"
#define NGX_CONF_OLDFILE NGX_ROOT_PATH "/conf/nginx_old.conf"
#define NGX_BIN_FILE NGX_ROOT_PATH "/sbin/nginx"

PUBLIC int StartNginxServer();
PUBLIC int StopNginxServer();
PUBLIC int RestartNginxServer();
PUBLIC int MVConfFile_now_to_old();
PUBLIC int MVConfFile_tmp_to_now();


PUBLIC int generate_default_ngx_conf();


#ifdef __cplusplus
}
#endif
#endif /* _h_GOAHEAD */


