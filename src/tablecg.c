/*
 * TABLECG.C
 *
 *	This is used to synthesize the table structure surrounding the site
 *	pages, to highlight Horizontal or Vertical selection features, and
 *	to track selections by modifying embedded LOCALLINK() directives.
 *
 *
 * $DragonFly: site/src/tablecg.c,v 1.5 2003/10/26 20:42:25 dillon Exp $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

static int VerboseOpt;
static const char *FilePath;
static const char *FileName;
static const char *DirPath;
static const char *DirName;
static const char *ParentDir;
static char *Av0;
static char *OBuf;
static int  OSize;
static int  OMax;
static char *Title;

static void generate_side_headers(char *, char * []);
static void process_vars(const char *ptr, int len);
static void process_command(char *cmd, char *args);
static void *safe_malloc(int bytes);
static char *safe_strdup(const char *str);
static void read_all(int fd, void *buf, int n);
static void buildout(const char *ptr, int len);
static void buildflush(void);
static const char *choppath(const char *path);
static const char *filecomp(const char *path);

char *Main[] = {
    "bugs.cgi",
    "download.cgi",
    "forums.cgi",
    "index.cgi",
    "mascot.cgi",
    NULL
};

char *Goals[] = {
    "caching.cgi",
    "index.cgi",
    "iomodel.cgi",
    "messaging.cgi",
    "packages.cgi",
    "threads.cgi",
    "userapi.cgi",
    "vfsmodel.cgi",
    NULL
};

char *Status[] = {
        "diary.cgi",
        "index.cgi",
        NULL
};


int
main(int ac, char **av)
{
    FILE *fi;
    char *base;
    int i;

    /*
     * Process options
     */
    Av0 = av[0];
    for (i = 1; i < ac; ++i) {
	char *ptr = av[i];
	if (*ptr != '-') {
	    FilePath = ptr;
	    continue;
	}
	ptr += 2;
	switch(ptr[-1]) {
	case 'v':
	    VerboseOpt = 1;
	    break;
	}
    }

    /*
     * Output headers and HTML start.
     */
    printf("Content-Type: text/html\r\n");
    printf("\r\n");

    if (FilePath == NULL) {
	fprintf(stderr, "%s: no file specified\n", av[0]);
	exit(1);
    }
    FileName = filecomp(FilePath);
    DirPath = choppath(FilePath);
    DirName = filecomp(DirPath);
    ParentDir = choppath(DirPath);

    /*
     * Process arguments
     */
    if ((base = getenv("CONTENT_LENGTH")) != NULL) {
	int l = strtol(base, NULL, 10);
	if (l < 0 || l > 1000000) {
	    fprintf(stderr, "%s: bad length %d processing %s\n", 
		av[0], l, FilePath);
	    exit(1);
	}
	base = safe_malloc(l + 1);
	base[l] = 0;
	read_all(0, base, l) ;
	process_vars(base, l);
	free(base);
    }
    if ((base = getenv("QUERY_STRING")) != NULL)
	process_vars(base, strlen(base));
    fflush(stdout);

    /*
     * Generate table structure
     */
    printf("<HTML>\n");
    printf("<HEAD>\n");
    printf("<TITLE></TITLE>\n");
    printf("<LINK REL=\"stylesheet\" HREF=\"/stylesheet.css\" TYPE=\"text/css\">");
    printf("</HEAD>\n");
    printf("<BODY>\n");
    /*
     * Process body
     */
    if (FilePath[0] && (fi = fopen(FilePath, "r")) != NULL) {
	char buf[256];
	char *las;
	char *ptr;

	while (fgets(buf, sizeof(buf), fi) != NULL) {
	    if (buf[0] == '#')
		continue;
	    las = ptr = buf;
	    while ((ptr = strchr(ptr, '$')) != NULL) {
		int i;
		int j;

		++ptr;

		for (i = 0; isalpha(ptr[i]); ++i)
		    ;
		if (i > 0 && ptr[i] == '(') {
		    for (j = i + 1; ptr[j] && ptr[j] != ')'; ++j)
			;
		    if (ptr[j] == ')') {
			buildout(las, ptr - las - 1);
			ptr[i] = 0;
			ptr[j] = 0;
			process_command(ptr, ptr + i + 1);
			las = ptr + j + 1;
			ptr = las;
		    }
		}
	    }
	    buildout(las, strlen(las));
	}
	fclose(fi);
    }
    printf("<TABLE BORDER=0 WIDTH=100%% BGCOLOR=\"#FFFFFF\">\n");
    printf("<TR><TD ALIGN=CENTER COLSPAN=2>");

    printf("<TABLE BORDER=0 WIDTH=100%% BGCOLOR=\"#FFFFFF\">\n");
    printf("<TR><TD VALIGN=\"bottom\" ALIGN=\"right\">");

	if (Title) {
	    printf("<H2>%s</H2>", Title);
	}
    printf("</TD><TD ALIGN=\"right\"><IMG SRC=\"/smalldf.jpg\"></TD>");
    printf("</TR><TR><TD COLSPAN=\"2\"><HR></TD>");
    printf("</TR></TABLE>");

    printf("</TD></TR>\n");
    printf("<TR><TD VALIGN=top WIDTH=\"150\">");

    generate_side_headers("Main", Main);
    generate_side_headers("Goals", Goals);
    generate_side_headers("Status", Status);

    printf("</TD><TD WIDTH=100%%  BGCOLOR=\"#ffffff\">");
    fflush(stdout);
    buildflush();
    printf("<PRE>\n");
    fflush(stdout);
    printf("</PRE>\n");

    /*
     * Finish table structure and add terminators.
     */
    printf("</TD></TR></TABLE>\n");
    printf("</BODY>\n");
    printf("</HTML>\n");
    return(0);
}

/*
 * The menu is synthesized from arrays above listing the files
 * we want to have available in the side menu.  A peek at the 
 * filesystem is needed so we know where we are, and how to 
 * format accordingly.
 */
static void
generate_side_headers(char *section, char *files[])
{
    int len;
    const char *ptr;
    int i;
    const char *fileclass = "";

    printf("\n<TABLE BORDER=\"0\" CELLPADDING=\"4\" WIDTH=\"100%%\">\n");
    printf("\t<TR>");
    printf("<TD><H2><A HREF=\"../%s\">%s</A></H2>", section, section);

    printf("</TD></TR>\n\t<TR><TD>\n<TABLE BORDER=\"0\" WIDTH=\"100%%\">\n");

    for (i = 0; files[i] != NULL; i++) {
        if ((strcmp(files[i], FileName) == 0) &&
        (strcmp(section, DirName) == 0) )
            fileclass = " CLASS=\"topLevelSelected\"";
        else
            fileclass = " CLASS=\"topLevel\"";
      
        if ((ptr = strchr(files[i], '.')) != NULL &&
            (strcmp(ptr + 1, "cgi") == 0 ||
            strcmp(ptr + 1, "html") == 0)
        ) {
            len = ptr - files[i];
            printf("\t<TR><TD%s><A HREF=\"/%s/%s\">%*.*s</A></TD></TR>\n",
            fileclass,
            section, 
            files[i], len, len, files[i]);
        }

    }
    printf("</TABLE>\n</TD>");
    printf("</TR>\n\t<TR><TD WIDTH=100%%></TD></TR>\n");
    printf("</TABLE>\n");
}

static void
process_vars(const char *ptr, int len)
{
}

static void
process_command(char *cmd, char *args)
{
    if (strcmp(cmd, "TITLE") == 0) {
	Title = safe_strdup(args);
    }
}

static void
read_all(int fd, void *buf, int n)
{
    int r;
    int t = 0;

    while (n > 0) {
	r = read(fd, buf, n);
	if (r < 0) {
	    fprintf(stderr, "%s: post read failed %d/%d: %s page %s\n",
		Av0, t, n, strerror(errno), FilePath);
	    exit(1);
	}
	n -= r;
	buf = (char *)buf + r;
    }
}

static void
buildout(const char *ptr, int len)
{
    if (OBuf == NULL) {
	OMax = 1024*1024;
	OBuf = malloc(OMax);
    }
    if (OSize + len > OMax)
	len = OMax - OSize;
    bcopy(ptr, OBuf + OSize, len);
    OSize += len;
}

static void
buildflush(void)
{
    if (OSize)
	fwrite(OBuf, OSize, 1, stdout);
}

static
const char *
choppath(const char *path)
{
    const char *ptr;
    char *nptr;

    if ((ptr = strrchr(path, '/')) != NULL) {
	nptr = malloc(ptr - path + 1);
	bcopy(path, nptr, ptr - path);
	nptr[ptr - path] = 0;
    } else {
	nptr = ".";
    }
    return(nptr);
}

static
const char *
filecomp(const char *path)
{
    const char *ptr;

    if ((ptr = strrchr(path, '/')) != NULL)
	return(ptr + 1);
    else
	return(path);
}

static void *
safe_malloc(int bytes)
{
    void *ptr = malloc(bytes);
    if (ptr == NULL) {
	fprintf(stderr, "%s: malloc() failed page %s\n", Av0, FilePath);
	exit(1);
    }
    return(ptr);
}

static char *
safe_strdup(const char *ptr)
{
    const char *str;

    if ((str = strdup(ptr)) == NULL) {
	fprintf(stderr, "%s: malloc() failed page %s\n", Av0, FilePath);
	exit(1);
    }
}

