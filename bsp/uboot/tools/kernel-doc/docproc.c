/*
 *	docproc is a simple preprocessor for the template files
 *      used as placeholders for the kernel internal documentation.
 *	docproc is used for documentation-frontend and
 *      dependency-generator.
 *	The two usages have in common that they require
 *	some knowledge of the .tmpl syntax, therefore they
 *	are kept together.
 *
 *	documentation-frontend
 *		Scans the template file and call kernel-doc for
 *		all occurrences of ![EIF]file
 *		Beforehand each referenced file is scanned for
 *		any symbols that are exported via these macros:
 *			EXPORT_SYMBOL(), EXPORT_SYMBOL_GPL(), &
 *			EXPORT_SYMBOL_GPL_FUTURE()
 *		This is used to create proper -function and
 *		-nofunction arguments in calls to kernel-doc.
 *		Usage: docproc doc file.tmpl
 *
 *	dependency-generator:
 *		Scans the template file and list all files
 *		referenced in a format recognized by make.
 *		Usage:	docproc depend file.tmpl
 *		Writes dependency information to stdout
 *		in the following format:
 *		file.tmpl src.c	src2.c
 *		The filenames are obtained from the following constructs:
 *		!Efilename
 *		!Ifilename
 *		!Dfilename
 *		!Ffilename
 *		!Pfilename
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

/* exitstatus is used to keep track of any failing calls to kernel-doc,
 * but execution continues. */
int exitstatus = 0;

typedef void DFL(char *);
DFL *defaultline;

typedef void FILEONLY(char * file);
FILEONLY *internalfunctions;
FILEONLY *externalfunctions;
FILEONLY *symbolsonly;
FILEONLY *findall;

typedef void FILELINE(char * file, char * line);
FILELINE * singlefunctions;
FILELINE * entity_system;
FILELINE * docsection;

#define MAXLINESZ     2048
#define MAXFILES      250
#define KERNELDOCPATH "tools/kernel-doc/"
#define KERNELDOC     "kernel-doc"
#define DOCBOOK       "-docbook"
#define LIST          "-list"
#define FUNCTION      "-function"
#define NOFUNCTION    "-nofunction"
#define NODOCSECTIONS "-no-doc-sections"

static char *srctree, *kernsrctree;

static char **all_list = NULL;
static int all_list_len = 0;

static void consume_symbol(const char *sym)
{
	int i;

	for (i = 0; i < all_list_len; i++) {
		if (!all_list[i])
			continue;
		if (strcmp(sym, all_list[i]))
			continue;
		all_list[i] = NULL;
		break;
	}
}

static int path_is_safe(const char *path)
{
	return path && *path &&
		!strstr(path, "../") &&
		!strstr(path, "/..") &&
		strcmp(path, "..");
}

static void build_path(char *buf, size_t buflen, const char *dir,
			 const char *path)
{
	int ret;

	ret = snprintf(buf, buflen, "%s/%s", dir, path);
	if (ret < 0 || (size_t)ret >= buflen) {
		fprintf(stderr, "docproc: path too long: %s/%s\n", dir, path);
		exit(1);
	}
}

static void *realloc_array(void *ptr, size_t nmemb, size_t size)
{
	if (size && nmemb > ((size_t)-1) / size)
		return NULL;

	return realloc(ptr, nmemb * size);
}

static void usage (void)
{
	fprintf(stderr, "Usage: docproc {doc|depend} file\n");
	fprintf(stderr, "Input is read from file.tmpl. Output is sent to stdout\n");
	fprintf(stderr, "doc: frontend when generating kernel documentation\n");
	fprintf(stderr, "depend: generate list of files referenced within file\n");
	fprintf(stderr, "Environment variable SRCTREE: absolute path to sources.\n");
	fprintf(stderr, "                     KBUILD_SRC: absolute path to kernel source tree.\n");
}

/*
 * Execute kernel-doc with parameters given in svec
 */
static void exec_kernel_doc(char **svec)
{
	pid_t pid;
	int ret;
	char real_filename[PATH_MAX + 1];
	/* Make sure output generated so far are flushed */
	fflush(stdout);
	switch (pid=fork()) {
		case -1:
			perror("fork");
			exit(1);
		case  0:
			build_path(real_filename, sizeof(real_filename),
				   kernsrctree, KERNELDOCPATH KERNELDOC);
			execvp(real_filename, svec);
			fprintf(stderr, "exec ");
			perror(real_filename);
			exit(1);
		default:
			waitpid(pid, &ret ,0);
	}
	if (WIFEXITED(ret))
		exitstatus |= WEXITSTATUS(ret);
	else
		exitstatus = 0xff;
}

/* Types used to create list of all exported symbols in a number of files */
struct symbols
{
	char *name;
};

struct symfile
{
	char *filename;
	struct symbols *symbollist;
	int symbolcnt;
};

struct symfile symfilelist[MAXFILES];
int symfilecnt = 0;

static void add_new_symbol(struct symfile *sym, char * symname)
{
	struct symbols *list;

	list = realloc_array(sym->symbollist, sym->symbolcnt + 1,
			       sizeof(*list));
	if (!list) {
		perror("docproc");
		exit(1);
	}
	sym->symbollist = list;
	sym->symbollist[sym->symbolcnt++].name = strdup(symname);
}

/* Add a filename to the list */
static struct symfile * add_new_file(char * filename)
{
	if (symfilecnt >= MAXFILES) {
		fprintf(stderr, "docproc: too many files referenced\n");
		exit(1);
	}
	symfilelist[symfilecnt++].filename = strdup(filename);
	return &symfilelist[symfilecnt - 1];
}

/* Check if file already are present in the list */
static struct symfile * filename_exist(char * filename)
{
	int i;
	for (i=0; i < symfilecnt; i++)
		if (strcmp(symfilelist[i].filename, filename) == 0)
			return &symfilelist[i];
	return NULL;
}

/*
 * List all files referenced within the template file.
 * Files are separated by tabs.
 */
static void adddep(char * file)		   { printf("\t%s", file); }
static void adddep2(char * file, char * line)     { line = line; adddep(file); }
static void noaction(char * line)		   { line = line; }
static void noaction2(char * file, char * line)   { file = file; line = line; }

/* Echo the line without further action */
static void printline(char * line)               { printf("%s", line); }

/*
 * Find all symbols in filename that are exported with EXPORT_SYMBOL &
 * EXPORT_SYMBOL_GPL (& EXPORT_SYMBOL_GPL_FUTURE implicitly).
 * All symbols located are stored in symfilelist.
 */
static void find_export_symbols(char * filename)
{
	FILE * fp;
	struct symfile *sym;
	char line[MAXLINESZ];
	if (filename_exist(filename) == NULL) {
		char real_filename[PATH_MAX + 1];

		if (!path_is_safe(filename)) {
			fprintf(stderr, "docproc: unsafe path '%s'\n", filename);
			exit(1);
		}
		build_path(real_filename, sizeof(real_filename),
			   srctree, filename);
		sym = add_new_file(filename);
		fp = fopen(real_filename, "r");
		if (fp == NULL)	{
			fprintf(stderr, "docproc: ");
			perror(real_filename);
			exit(1);
		}
		while (fgets(line, MAXLINESZ, fp)) {
			char *p;
			char *e;
			if (((p = strstr(line, "EXPORT_SYMBOL_GPL")) != NULL) ||
                            ((p = strstr(line, "EXPORT_SYMBOL")) != NULL)) {
				/* Skip EXPORT_SYMBOL{_GPL} */
				while (isalnum(*p) || *p == '_')
					p++;
				/* Remove parentheses & additional whitespace */
				while (isspace(*p))
					p++;
				if (*p != '(')
					continue; /* Syntax error? */
				else
					p++;
				while (isspace(*p))
					p++;
				e = p;
				while (isalnum(*e) || *e == '_')
					e++;
				*e = '\0';
				add_new_symbol(sym, p);
			}
		}
		fclose(fp);
	}
}

/*
 * Document all external or internal functions in a file.
 * Call kernel-doc with following parameters:
 * kernel-doc -docbook -nofunction function_name1 filename
 * Function names are obtained from all the src files
 * by find_export_symbols.
 * intfunc uses -nofunction
 * extfunc uses -function
 */
static void docfunctions(char * filename, char * type)
{
	int i,j;
	int symcnt = 0;
	int idx = 0;
	char **vec;
	size_t vec_count;

	for (i = 0; i < symfilecnt; i++)
		symcnt += symfilelist[i].symbolcnt;
	vec_count = 2 * symcnt + 5;
	vec = malloc(vec_count * sizeof(char *));
	if (vec == NULL) {
		perror("docproc: ");
		exit(1);
	}
	vec[idx++] = KERNELDOC;
	vec[idx++] = DOCBOOK;
	vec[idx++] = NODOCSECTIONS;
	for (i=0; i < symfilecnt; i++) {
		struct symfile * sym = &symfilelist[i];
		for (j=0; j < sym->symbolcnt; j++) {
			vec[idx++]     = type;
			consume_symbol(sym->symbollist[j].name);
			vec[idx++] = sym->symbollist[j].name;
		}
	}
	vec[idx++]     = filename;
	vec[idx] = NULL;
	printf("<!-- %s -->\n", filename);
	exec_kernel_doc(vec);
	fflush(stdout);
	free(vec);
}
static void intfunc(char * filename) {	docfunctions(filename, NOFUNCTION); }
static void extfunc(char * filename) { docfunctions(filename, FUNCTION);   }

/*
 * Document specific function(s) in a file.
 * Call kernel-doc with the following parameters:
 * kernel-doc -docbook -function function1 [-function function2]
 */
static void singfunc(char * filename, char * line)
{
	char *vec[200]; /* Enough for specific functions */
        int i, idx = 0;
        int startofsym = 1;
	vec[idx++] = KERNELDOC;
	vec[idx++] = DOCBOOK;

        /* Split line up in individual parameters preceded by FUNCTION */
        for (i=0; line[i]; i++) {
                if (isspace(line[i])) {
                        line[i] = '\0';
                        startofsym = 1;
                        continue;
                }
                if (startofsym) {
                        startofsym = 0;
                        vec[idx++] = FUNCTION;
                        vec[idx++] = &line[i];
                }
        }
	for (i = 0; i < idx; i++) {
		if (strcmp(vec[i], FUNCTION))
			continue;
		consume_symbol(vec[i + 1]);
	}
	vec[idx++] = filename;
	vec[idx] = NULL;
	exec_kernel_doc(vec);
}

/*
 * Insert specific documentation section from a file.
 * Call kernel-doc with the following parameters:
 * kernel-doc -docbook -function "doc section" filename
 */
static void docsect(char *filename, char *line)
{
	char *vec[6]; /* kerneldoc -docbook -function "section" file NULL */
	char *s;

	for (s = line; *s; s++)
		if (*s == '\n')
			*s = '\0';

	if (asprintf(&s, "DOC: %s", line) < 0) {
		perror("asprintf");
		exit(1);
	}
	consume_symbol(s);
	free(s);

	vec[0] = KERNELDOC;
	vec[1] = DOCBOOK;
	vec[2] = FUNCTION;
	vec[3] = line;
	vec[4] = filename;
	vec[5] = NULL;
	exec_kernel_doc(vec);
}

static void find_all_symbols(char *filename)
{
	char *vec[4]; /* kerneldoc -list file NULL */
	pid_t pid;
	int ret, i, count, start;
	char real_filename[PATH_MAX + 1];
	int pipefd[2];
	char *data, *str;
	size_t data_len = 0;

	vec[0] = KERNELDOC;
	vec[1] = LIST;
	vec[2] = filename;
	vec[3] = NULL;

	if (pipe(pipefd)) {
		perror("pipe");
		exit(1);
	}

	switch (pid=fork()) {
		case -1:
			perror("fork");
			exit(1);
		case  0:
			close(pipefd[0]);
			dup2(pipefd[1], 1);
			build_path(real_filename, sizeof(real_filename),
				   kernsrctree, KERNELDOCPATH KERNELDOC);
			execvp(real_filename, vec);
			fprintf(stderr, "exec ");
			perror(real_filename);
			exit(1);
		default:
		{
			char *new_data;

			close(pipefd[1]);
			data = malloc(4096);
			if (!data) {
				perror("docproc");
				exit(1);
			}
			do {
				while ((ret = read(pipefd[0],
						   data + data_len,
						   4096)) > 0) {
					data_len += ret;
					new_data = realloc_array(data,
							 data_len + 4096, 1);
					if (!new_data) {
						perror("docproc");
						exit(1);
					}
					data = new_data;
				}
			} while (ret == -1 && errno == EAGAIN);
			if (ret < 0) {
				perror("read");
				exit(1);
			}
			waitpid(pid, &ret ,0);
		}
	}
	if (WIFEXITED(ret))
		exitstatus |= WEXITSTATUS(ret);
	else
		exitstatus = 0xff;

	count = 0;
	/* poor man's strtok, but with counting */
	for (i = 0; i < data_len; i++) {
		if (data[i] == '\n') {
			count++;
			data[i] = '\0';
		}
	}
	start = all_list_len;
	all_list_len += count;
	str = data;
	if (count) {
		char **new_list;

		new_list = realloc_array(all_list, all_list_len, sizeof(char *));
		if (!new_list) {
			perror("docproc");
			exit(1);
		}
		all_list = new_list;
	}
	for (i = 0; i < data_len && start != all_list_len; i++) {
		if (data[i] == '\0') {
			all_list[start] = str;
			str = data + i + 1;
			start++;
		}
	}
}

/*
 * Parse file, calling action specific functions for:
 * 1) Lines containing !E
 * 2) Lines containing !I
 * 3) Lines containing !D
 * 4) Lines containing !F
 * 5) Lines containing !P
 * 6) Lines containing !C
 * 7) Default lines - lines not matching the above
 */
static void parse_file(FILE *infile)
{
	char line[MAXLINESZ];
	char * s;
	while (fgets(line, MAXLINESZ, infile)) {
		if (line[0] == '!') {
			s = line + 2;
			switch (line[1]) {
				case 'E':
					while (*s && !isspace(*s)) s++;
					*s = '\0';
					externalfunctions(line+2);
					break;
				case 'I':
					while (*s && !isspace(*s)) s++;
					*s = '\0';
					internalfunctions(line+2);
					break;
				case 'D':
					while (*s && !isspace(*s)) s++;
                                        *s = '\0';
                                        symbolsonly(line+2);
                                        break;
				case 'F':
					/* filename */
					while (*s && !isspace(*s)) s++;
					*s++ = '\0';
                                        /* function names */
					while (isspace(*s))
						s++;
					singlefunctions(line +2, s);
					break;
				case 'P':
					/* filename */
					while (*s && !isspace(*s)) s++;
					*s++ = '\0';
					/* DOC: section name */
					while (isspace(*s))
						s++;
					docsection(line + 2, s);
					break;
				case 'C':
					while (*s && !isspace(*s)) s++;
					*s = '\0';
					if (findall)
						findall(line+2);
					break;
				default:
					defaultline(line);
			}
		} else {
			defaultline(line);
		}
	}
	fflush(stdout);
}


int main(int argc, char *argv[])
{
	FILE * infile;
	int i;

	srctree = getenv("SRCTREE");
	if (!srctree)
		srctree = getcwd(NULL, 0);
	kernsrctree = getenv("KBUILD_SRC");
	if (!kernsrctree || !*kernsrctree)
		kernsrctree = srctree;
	if (argc != 3) {
		usage();
		exit(1);
	}
	/* Open file, exit on error */
	if (!path_is_safe(argv[2])) {
		fprintf(stderr, "docproc: unsafe path '%s'\n", argv[2]);
		exit(2);
	}
	infile = fopen(argv[2], "r");
        if (infile == NULL) {
                fprintf(stderr, "docproc: ");
                perror(argv[2]);
                exit(2);
        }

	if (strcmp("doc", argv[1]) == 0) {
		/* Need to do this in two passes.
		 * First pass is used to collect all symbols exported
		 * in the various files;
		 * Second pass generate the documentation.
		 * This is required because some functions are declared
		 * and exported in different files :-((
		 */
		/* Collect symbols */
		defaultline       = noaction;
		internalfunctions = find_export_symbols;
		externalfunctions = find_export_symbols;
		symbolsonly       = find_export_symbols;
		singlefunctions   = noaction2;
		docsection        = noaction2;
		findall           = find_all_symbols;
		parse_file(infile);

		/* Rewind to start from beginning of file again */
		fseek(infile, 0, SEEK_SET);
		defaultline       = printline;
		internalfunctions = intfunc;
		externalfunctions = extfunc;
		symbolsonly       = printline;
		singlefunctions   = singfunc;
		docsection        = docsect;
		findall           = NULL;

		parse_file(infile);

		for (i = 0; i < all_list_len; i++) {
			if (!all_list[i])
				continue;
			fprintf(stderr, "Warning: didn't use docs for %s\n",
				all_list[i]);
		}
	} else if (strcmp("depend", argv[1]) == 0) {
		/* Create first part of dependency chain
		 * file.tmpl */
		printf("%s\t", argv[2]);
		defaultline       = noaction;
		internalfunctions = adddep;
		externalfunctions = adddep;
		symbolsonly       = adddep;
		singlefunctions   = adddep2;
		docsection        = adddep2;
		findall           = adddep;
		parse_file(infile);
		printf("\n");
	} else {
		fprintf(stderr, "Unknown option: %s\n", argv[1]);
		exit(1);
	}
	fclose(infile);
	fflush(stdout);
	return exitstatus;
}
