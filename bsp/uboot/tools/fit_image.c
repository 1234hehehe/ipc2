/*
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 *
 * Updated-by: Prafulla Wadaskar <prafulla@marvell.com>
 *		FIT image specific code abstracted from mkimage.c
 *		some functions added to address abstraction
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "mkimage.h"
#include <image.h>
#include <sys/wait.h>
#include <u-boot/crc.h>

static image_header_t header;

static int fit_copy_file(struct mkimage_params *params, const char *src,
			 const char *dst)
{
	char buf[4096];
	int ifd, ofd;
	ssize_t len;

	ifd = open(src, O_RDONLY | O_BINARY);
	if (ifd < 0) {
		fprintf(stderr, "%s: Can't open %s: %s\n",
			params->cmdname, src, strerror(errno));
		return -1;
	}

	ofd = open(dst, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
	if (ofd < 0) {
		fprintf(stderr, "%s: Can't create %s: %s\n",
			params->cmdname, dst, strerror(errno));
		close(ifd);
		return -1;
	}

	while ((len = read(ifd, buf, sizeof(buf))) > 0) {
		ssize_t written = 0;

		while (written < len) {
			ssize_t ret = write(ofd, buf + written, len - written);

			if (ret < 0) {
				fprintf(stderr, "%s: Can't write %s: %s\n",
					params->cmdname, dst, strerror(errno));
				close(ofd);
				close(ifd);
				return -1;
			}
			written += ret;
		}
	}

	if (len < 0) {
		fprintf(stderr, "%s: Can't read %s: %s\n",
			params->cmdname, src, strerror(errno));
		close(ofd);
		close(ifd);
		return -1;
	}

	close(ofd);
	close(ifd);

	return 0;
}

static int fit_run_dtc(struct mkimage_params *params, const char *tmpfile)
{
	char *dtc_args, *tok, *saveptr;
	char *argv[64];
	pid_t pid;
	int ofd, argc = 0, status;

	dtc_args = strdup(params->dtc ? params->dtc : "");
	if (!dtc_args) {
		fprintf(stderr, "%s: Out of memory\n", params->cmdname);
		return -1;
	}

	argv[argc++] = MKIMAGE_DTC;
	for (tok = strtok_r(dtc_args, " \t", &saveptr);
	     tok && argc < ARRAY_SIZE(argv) - 2;
	     tok = strtok_r(NULL, " \t", &saveptr))
		argv[argc++] = tok;
	argv[argc++] = params->datafile;
	argv[argc] = NULL;

	ofd = open(tmpfile, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
	if (ofd < 0) {
		fprintf(stderr, "%s: Can't create %s: %s\n",
			params->cmdname, tmpfile, strerror(errno));
		free(dtc_args);
		return -1;
	}

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "%s: Can't fork: %s\n",
			params->cmdname, strerror(errno));
		close(ofd);
		free(dtc_args);
		return -1;
	}

	if (!pid) {
		if (dup2(ofd, STDOUT_FILENO) < 0)
			_exit(EXIT_FAILURE);
		close(ofd);
		execvp(MKIMAGE_DTC, argv);
		_exit(EXIT_FAILURE);
	}

	close(ofd);
	free(dtc_args);

	if (waitpid(pid, &status, 0) < 0) {
		fprintf(stderr, "%s: Can't wait for dtc: %s\n",
			params->cmdname, strerror(errno));
		return -1;
	}

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		fprintf(stderr, "%s: dtc failed\n", params->cmdname);
		return -1;
	}

	return 0;
}

static int fit_verify_header (unsigned char *ptr, int image_size,
			struct mkimage_params *params)
{
	return fdt_check_header ((void *)ptr);
}

static int fit_check_image_types (uint8_t type)
{
	if (type == IH_TYPE_FLATDT)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

int mmap_fdt(struct mkimage_params *params, const char *fname, void **blobp,
		struct stat *sbuf)
{
	void *ptr;
	int fd;

	/* Load FIT blob into memory (we need to write hashes/signatures) */
	fd = open(fname, O_RDWR | O_BINARY);

	if (fd < 0) {
		fprintf(stderr, "%s: Can't open %s: %s\n",
			params->cmdname, fname, strerror(errno));
		unlink(fname);
		return -1;
	}

	if (fstat(fd, sbuf) < 0) {
		fprintf(stderr, "%s: Can't stat %s: %s\n",
			params->cmdname, fname, strerror(errno));
		unlink(fname);
		return -1;
	}

	ptr = mmap(0, sbuf->st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "%s: Can't read %s: %s\n",
			params->cmdname, fname, strerror(errno));
		unlink(fname);
		return -1;
	}

	/* check if ptr has a valid blob */
	if (fdt_check_header(ptr)) {
		fprintf(stderr, "%s: Invalid FIT blob\n", params->cmdname);
		unlink(fname);
		return -1;
	}

	*blobp = ptr;
	return fd;
}

/**
 * fit_handle_file - main FIT file processing function
 *
 * fit_handle_file() runs dtc to convert .its to .itb, includes
 * binary data, updates timestamp property and calculates hashes.
 *
 * datafile  - .its file
 * imagefile - .itb file
 *
 * returns:
 *     only on success, otherwise calls exit (EXIT_FAILURE);
 */
static int fit_handle_file (struct mkimage_params *params)
{
	char tmpfile[MKIMAGE_MAX_TMPFILE_LEN];
	int tfd, destfd = 0;
	void *dest_blob = NULL;
	struct stat sbuf;
	void *ptr;
	off_t destfd_size = 0;

	/* Flattened Image Tree (FIT) format  handling */
	debug ("FIT format handling\n");

	/* call dtc to include binary properties into the tmp file */
	if (strlen (params->imagefile) +
		strlen (MKIMAGE_TMPFILE_SUFFIX) + 1 > sizeof (tmpfile)) {
		fprintf (stderr, "%s: Image file name (%s) too long, "
				"can't create tmpfile",
				params->imagefile, params->cmdname);
		return (EXIT_FAILURE);
	}
	sprintf (tmpfile, "%s%s", params->imagefile, MKIMAGE_TMPFILE_SUFFIX);

	/* We either compile the source file, or use the existing FIT image */
	if (params->datafile) {
		debug("Trying to execute \"%s\"\n", MKIMAGE_DTC);
		if (fit_run_dtc(params, tmpfile))
			goto err_system;
	} else {
		if (fit_copy_file(params, params->imagefile, tmpfile))
			goto err_system;
	}

	if (params->keydest) {
		destfd = mmap_fdt(params, params->keydest, &dest_blob, &sbuf);
		if (destfd < 0)
			goto err_keydest;
		destfd_size = sbuf.st_size;
	}

	tfd = mmap_fdt(params, tmpfile, &ptr, &sbuf);
	if (tfd < 0)
		goto err_mmap;

	/* set hashes for images in the blob */
	if (fit_add_verification_data(params->keydir,
				      dest_blob, ptr, params->comment,
				      params->require_keys)) {
		fprintf(stderr, "%s Can't add hashes to FIT blob\n",
			params->cmdname);
		goto err_add_hashes;
	}

	/* for first image creation, add a timestamp at offset 0 i.e., root  */
	if (params->datafile && fit_set_timestamp(ptr, 0, sbuf.st_mtime)) {
		fprintf (stderr, "%s: Can't add image timestamp\n",
				params->cmdname);
		goto err_add_timestamp;
	}
	debug ("Added timestamp successfully\n");

	munmap ((void *)ptr, sbuf.st_size);
	close (tfd);
	if (dest_blob) {
		munmap(dest_blob, destfd_size);
		close(destfd);
	}

	if (rename (tmpfile, params->imagefile) == -1) {
		fprintf (stderr, "%s: Can't rename %s to %s: %s\n",
				params->cmdname, tmpfile, params->imagefile,
				strerror (errno));
		unlink (tmpfile);
		unlink (params->imagefile);
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);

err_add_timestamp:
err_add_hashes:
	munmap(ptr, sbuf.st_size);
err_mmap:
	if (dest_blob)
		munmap(dest_blob, destfd_size);
err_keydest:
err_system:
	unlink(tmpfile);
	return -1;
}

static int fit_check_params (struct mkimage_params *params)
{
	return	((params->dflag && (params->fflag || params->lflag)) ||
		(params->fflag && (params->dflag || params->lflag)) ||
		(params->lflag && (params->dflag || params->fflag)));
}

static struct image_type_params fitimage_params = {
	.name = "FIT Image support",
	.header_size = sizeof(image_header_t),
	.hdr = (void*)&header,
	.verify_header = fit_verify_header,
	.print_header = fit_print_contents,
	.check_image_type = fit_check_image_types,
	.fflag_handle = fit_handle_file,
	.set_header = NULL,	/* FIT images use DTB header */
	.check_params = fit_check_params,
};

void init_fit_image_type (void)
{
	mkimage_register (&fitimage_params);
}
