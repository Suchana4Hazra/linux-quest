/* Filename: myfsv1.c */

/* If the resulting executable file would work as mymkfs, mycopyTo, mycopyFrom, and myrm depending on
   how it is named. */

/* Problem statement */

/* Let the linux file (viewed as collection of blocks) that would store multiple other linux files be dd1.

The following figure depicts how dd1 can be visualised.

dd1

The 1st 8 blocks (8 x 4096 bytes) of dd1 will store the metadata (name and size) of the linux files contained in dd1. Remaining 2048 blocks of dd1 are the data blocks of the contained files. The files contained in dd1 satisfy the following constraints.

Size of each file can at most 4096 bytes requiring 1 block of dd1.
Name of each can have 12 characters at the most.

To store the metedata of every linux file contained in dd1, 16 bytes (12 for the name and 4 for the size). Since each file needs one block, at most 2048 files can be stored in dd1. In the 1st 8 blocks too, metadata of 2048 files can be stored,  that is, (8 x 4096) / 16 = 2048, 

ith ( i = 0...2047)  data block in dd1 corresponds to the ith metada in the 8 metedata blocks.

If the name part in metadata starts with null character, it implies that there is no file there.

You have to implement the following commands.

1. mymkfs dd1 [makes dd1 ready for storing files]
2. mycopyTo <linux file> dd1 [copies a linux file to dd1]
3. mycopyFrom <file name>@dd1 [copies a file from dd1 to a linux file of same name.]
4.  myrm <file name>@dd1 [removes a file from dd1]

Your code should properly indented and documented.

*/

#include <stdio.h>
#include <stdio.h>
#include <string.h> /* strcmp(), strrchr() */
#include <stdlib.h> /* exit() */
#include <fcntl.h> /* open() */
#include <errno.h> /* perror() */
#include <unistd.h> /* write() close() */
#include <sys/stat.h> /* stat() */
#include <sys/sysmacros.h> /* stat() */
#include <stdint.h> /* stat() */




#define BS 4096
#define BNO 2048
#define NOFILES 2048
#define FNLEN 12

char buf[4096];
char sbuf[8*4096];

/* prototypes of function define later */
int mymkfs(const char *fname); /* returns 0 if successfull, -1 if error */
int mycopyFrom(char *mfname, char *fname);
int mycopyTo(char *fname, char *mfname);
int myrm(char *);
int myreadSBlocks(int fd, char *sbuf); /* returns 0 if successfull, -1 if error */
int mywriteSBlocks(int fd, char *sbuf);
int myreadBlock(int fd, int bno, char *buf); /* returns 0 if successfull, -1 if error */
int mywriteBlock(int fd, int bno, char *buf); /* returns 0 if successfull, -1 if error */

int main(int argc, char *argv[]) {
	char *basename;
	int flag;

	basename = strrchr(argv[0], '/');
	if (basename != NULL) {
		basename++;
	} else {
		basename = argv[0];
	}

	if (strcmp(basename, "mymkfs") == 0) {
		if(argc != 2) {

			fprintf(stderr, "Usage: %s <linux file>\n", argv[0]);
			exit(1);
		}
		flag = mymkfs(argv[1]);

		if (flag != 0) { /* mymkfs() has failed for some reason */
			fprintf(stderr, "%s Failed!\n", argv[0]);
		}

	}  else if (strcmp(basename, "mycopyTo") == 0) {
		if (argc != 3) {
                        fprintf(stderr, "Usage: %s <linux file name> <storage file name>\n", argv[0]);
                        exit(1);
                }
                flag = mycopyTo(argv[1], argv[2]);
		if (flag != 0) { /* mycopyTo() has failed for some reason */
			fprintf(stderr, "%s Failed!\n", argv[0]);
		}
	}  else if (strcmp(basename, "mycopyFrom") == 0) {
		if (argc != 3) {
                        fprintf(stderr, "Usage: %s <myfile name>@<storage file name> <linux file name>\n", argv[0]);
                        exit(1);
                }
                flag = mycopyFrom(argv[1], argv[2]);
		if (flag != 0) { /* mycopyFrom() has failed for some reason */
			fprintf(stderr, "%s Failed!\n", argv[0]);
		}

	}  else if (strcmp(basename, "myrm") == 0) {
		if (argc != 2) {
                        fprintf(stderr, "Usage: %s <myfile name>@<storage file name>\n", argv[0]);
                        exit(1);
                }
                flag = myrm(argv[1]);
		if (flag != 0) { /* myrm() has failed for some reason */
			fprintf(stderr, "%s Failed!\n", argv[0]);
		}


	}  else {
		fprintf(stderr,"%s: Command not found!\n", argv[0]);
	}
}



int mymkfs(const char *fname) {
	int fd;
	int i;
	int flag;
	fd = open(fname, O_CREAT | O_WRONLY, S_IRWXU);
	if (fd == -1) {
		fprintf(stderr,"%s: ", fname);
		perror("File cannot be opened for writing");
		return (-1);
	}
	/* initialize buf */
	// void *memset(void s[.n], int c, size_t n);
	memset(buf, 0, BS);
	for (i=0; i < BNO + 8; i++) {
		flag = write(fd, buf, BS);
		if(flag == -1) {
			fprintf(stderr,"%s: ", fname);
			perror("File write failed!");
			return (-1);
		}
	}
	return (0);

}
int mycopyTo(char *fname, char *mfname) {
	/* linux file fname to be copied to mfname on myfs */
	int fd;
	int fdTo;
	int i;
	int flag;       
	int hole;
        struct stat sb;

	//int stat(const char *restrict pathname, struct stat *restrict statbuf);
	flag =  stat(fname, &sb);
	if (flag == -1) { /* stat() failed */
		fprintf(stderr,"File %s ", fname);
		perror("stat() failed: ");
		return (-1);
	}

	if (strlen(fname) > FNLEN){
		// Error
		fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, mfname);
		fprintf(stderr, "Name is longer than %d!\n", FNLEN);
		return (-1);
	} else if (sb.st_size > BS) {
		// Error
		fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, mfname);
		fprintf(stderr, "File size %d is bigger than %d!\n", (int)(sb.st_size), BS);
		return (-1);
	}

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr,"%s: ", fname);
		perror("Cannot be opened for reading: ");
		return (-1);
	}

	fdTo = open(mfname, O_RDWR);
	if (fdTo == -1) {
		fprintf(stderr,"%s: ", mfname);
		perror("Cannot be opened for writing: ");
		return (-1);
	}

	/* initialize sbuf */
	flag = myreadSBlocks(fdTo, sbuf);

	/* check whether fname exists in myfs */
	hole = -1;
	for (i = 0; i < NOFILES; i++) { /* check all the myfile names in 16 byte myfile descriptors.*/
		if (sbuf[i*16] == 0) {
			hole = i;
		}
		if (strncmp(fname, &(sbuf[i*16]), FNLEN) == 0) {
			break;
		}
	}

	if (i >= NOFILES) {
		strcpy(&(sbuf[hole*16]), fname); /* copy the fname in the myfile descriptor */
		*((int *)&(sbuf[hole*16 + 12])) = (int)(sb.st_size); /* copy the fname size in the myfile descriptor */
		//sprintf(&(sbuf[hole*16 + 12]), "%d", sb.st_size);

	} else {
		/** Error: File exists */
		fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, mfname);
		fprintf(stderr, "File already exists!\n");
		return (-1);
	}

	flag = read(fd, buf, BS);
	if(flag == -1) { /* read() failed */
		fprintf(stderr,"%s: ", fname);
		perror("File read failed!");
		return (-1);
	}
	
	//int mywriteBlock(int fd, int bno, char *buf) 
	flag = mywriteBlock(fdTo, 8+hole, buf);
	if (flag == -1) {
		fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, mfname);
		fprintf(stderr, "mywriteBlock() failed!\n");
		return (-1);
	}
	
	//int mywriteSBlocks(int fd, char *sbuf) 
	flag = mywriteSBlocks(fdTo, sbuf);
	if (flag == -1) {
		fprintf(stderr, "File %s cannot be copied to myfs on %s!\n", fname, mfname);
		fprintf(stderr, "mywriteSBlocks() failed!\n");
		close(fdTo);
		return (-1);
	}
	close(fd);
	close(fdTo);
	return (0);

}
int mycopyFrom(char *mfname, char *fname) {
	/* myfile mfname to be copied to linux file fname*/
	/* mfname will be of the from <myfile name>@<myfs file name> */
	int fd;
	int fdFrom;
	int i;
	int flag;       
        struct stat sb;
	char *myfsname;
	char *myfilename;
	int myfilesize;

	// fprintf(stderr, "Debug: Command: mycopyFrom %s %s!\n", mfname, fname);
	myfilename = mfname;
	myfsname = strchr(mfname, '@');
	if (myfsname != NULL) {
		*myfsname = '\0';
		myfsname++;
	} else {
		fprintf(stderr, "%s should be of the form <myfile name>@<myfs file name>\n", mfname);
		return(-1);
	}


	fd = open(fname, O_CREAT | O_WRONLY, S_IRWXU);
	if (fd == -1) {
		fprintf(stderr,"%s: ", fname);
		perror("Cannot be opened for writing");
		return (-1);
	}

	fdFrom = open(myfsname, O_RDWR);
	if (fdFrom == -1) {
		fprintf(stderr,"%s: ", myfsname);
		perror("Cannot be opened for reading: ");
		return (-1);
	}

	/* initialize sbuf */
	flag = myreadSBlocks(fdFrom, sbuf);

	/* check whether myfilename exists in myfs */
	for (i = 0; i < NOFILES; i++) { /* check all the myfile names in 16 byte myfile descriptors.*/
		if (strncmp(myfilename, &(sbuf[i*16]), FNLEN) == 0) { /* myfile found */
			break;
		}
	}

	if (i >= NOFILES) { /* myfile not found */
		/** Error: File not found at myfs */
		fprintf(stderr, "File %s cannot be found in  myfs on %s!\n", myfilename, myfsname);
		return (-1);
	}

	/* Read myfile size from the descriptor */
	myfilesize = *((int *)&(sbuf[i*16 + 12]));

	/* Read myfile data */ 
	//int myreadBlock(int fd, int bno, char *buf) 
	flag = myreadBlock(fdFrom, 8+i, buf);
	if (flag == -1) {
		fprintf(stderr, "File %s cannot be read from myfs on %s!\n", myfilename, myfsname);
		fprintf(stderr, "myreadBlock() failed!\n");
		return (-1);
	}

	flag = write(fd, buf, myfilesize);
	if(flag == -1) { /* write() failed */
		fprintf(stderr,"%s: ", fname);
		perror("File write failed!");
		close(fd);
		return (-1);
	}
	
	close(fd);
	close(fdFrom);
	return (0);

}
int myrm(char *mfname) {
	/* myfile mfname to be removed */
	/* mfname will be of the from <myfile name>@<myfs file name> */
	int fdFrom;
	int i;
	int flag;       
	char *myfsname;
	char *myfilename;

	myfilename = mfname;
	myfsname = strchr(mfname, '@');
	if (myfsname != NULL) {
		*myfsname = '\0';
		myfsname++;
	} else {
		fprintf(stderr, "%s should be of the form <myfile name>@<myfs file name>\n", mfname);
		return(-1);
	}


	fdFrom = open(myfsname, O_RDWR);
	if (fdFrom == -1) {
		fprintf(stderr,"%s: ", myfsname);
		perror("Cannot be opened for reading-writing: ");
		return (-1);
	}

	/* initialize sbuf */
	flag = myreadSBlocks(fdFrom, sbuf);

	/* check whether myfilename exists in myfs at myfsname */
	for (i = 0; i < BNO; i++) { /* check all the myfile names in 16 byte myfile descriptors.*/
		if (strncmp(myfilename, &(sbuf[i*16]), FNLEN) == 0) { /* myfile found */
			break;
		}
	}


	if (i >= NOFILES) { /* myfile not found */
		/** Error: File not found at myfs */
		fprintf(stderr, "File %s cannot be found in  myfs on %s!\n", myfilename, myfsname);
		return (-1);
	}

	/* delete the file in the myfile descriptor */
	sbuf[i*16] = '\0';
	*((int *)&(sbuf[i*16 + 12])) = 0; /* make myfilename size in the myfile descriptor to be 0. optional */


	//int mywriteSBlocks(int fd, char *sbuf) 
	flag = mywriteSBlocks(fdFrom, sbuf);
	if (flag == -1) {
		fprintf(stderr, "File %s cannot be removed from myfs on %s!\n", myfilename, myfsname);
		fprintf(stderr, "mywriteSBlocks() failed!\n");
		close(fdFrom);
		return (-1);
	}
	close(fdFrom);
	return (0);

}


int myreadSBlocks(int fd, char *sbuf) {
	int i;
	int flag;
	flag = lseek(fd, 0, SEEK_SET); /* Going to the beginning of the file */
        if (flag == -1) {
                perror("lseek() at myreadSBlocks() fails: ");
                return (flag);
        }
	flag = 0;
	for (i = 0; i < 8 && flag != -1; i++) {
		flag = myreadBlock(fd, i, &(sbuf[i*BS]));
	}
	return (flag);
}
int mywriteSBlocks(int fd, char *sbuf) {
	int i;
	int flag;
	flag = lseek(fd, 0, SEEK_SET); /* Going to the beginning of the file */
        if (flag == -1) {
                perror("lseek() at mywriteSBlocks() fails: ");
                return (-1);
        }
	flag = 0;
	for (i = 0; i < 8 && flag != -1; i++) {
		flag = mywriteBlock(fd, i, &(sbuf[i*BS]));
	}
	return (flag);
}
int myreadBlock(int fd, int bno, char *buf) {
        int flag;
        flag = lseek(fd, bno * BS, SEEK_SET);
        if (flag == -1) {
                perror("lseek() at myreadBlock() fails: ");
                return (flag);
        }
        flag = read(fd, buf, BS);
        if (flag == -1) {
                perror("read() at myreadBlock() fails: ");
                return (-1);
        }
        return (0);
}

int mywriteBlock(int fd, int bno, char *buf) {
        int flag;
        flag = lseek(fd, bno * BS, SEEK_SET);
        if (flag == -1) {
                perror("lseek() at myreadBlock() fails: ");
                return (flag);
        }
        flag = write(fd, buf, BS);
        if (flag == -1) {
                perror("read() at myreadBlock() fails: ");
                return (-1);
        }
        return (0);
}
