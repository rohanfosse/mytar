#include "file.h"


void error(const char *fmt, ...)
{
va_list l;
va_start(l, fmt);
vfprintf(stderr, fmt, l);
va_end(l);
exit(-1);
}


void call_error(const char *func, ...)
{
error("%s: %s\n", func, strerror(errno));
}


int copie_fichier(int fd_src, int fd_target, int size)
{
int n, r_size;
char buf[BUF_SIZE];
while(size > 0)
{
r_size = (BUF_SIZE < size ? BUF_SIZE : size);
n = read(fd_src, buf, r_size);
if(n == -1) {
call_error("read");
}
if(n == 0) break;
if(write(fd_target, buf, n) != n) {
call_error("write");
}
size -= n;
}
}

char * my_readlink(const char *lnk)
{
static char path[_POSIX_PATH_MAX+1];
int n;
if((n = readlink(lnk, path, _POSIX_PATH_MAX)) == -1) {
call_error(lnk);
}
path[n] = '\0';
return path;
}


int find_file_in_list(const char *file, int argc, char **argv)
{
int i = -1;
while(argc-- > 0) {
i++;
if(strcmp(*(argv++), file) == 0) {
return i;
}
}
return i;
}


int fichiers_exist(const char *file, int argc, char **argv)
{
if(argc <= 0) return 1;
return find_file_in_list(file, argc, argv);
}


char * virer_premier_slash(const char *str)
{
static char path[_POSIX_PATH_MAX+1];
int i=0;
while(str[i] == '/') {
i++;
}
strcpy(path, str+i);
return path;
}


char * virer_dernier_slash(const char *str)
{
static char path[_POSIX_PATH_MAX+1];
int i = strlen(str)-1;
strcpy(path, str);
while(i>0 && path[i] == '/') {
path[i] = '\0';
i--;
}
return path;
}


int extract_next_file(int tfd, int argc, char **argv)
{
file_struct fs;
int read_val;
char * file_buf;
int fd;
if((read_val = read(tfd, &fs, sizeof(file_struct))) == -1) { // erreur lors de la lecture du fichier
call_error("read");
}
if(read_val == 0) // S'il est vide
return 0;
if(!fichiers_exist(fs.path, argc, argv))// Si les fichiers n'existent pas
return 1;
if(tar_cmd == TAR_LIST || (tar_flag & TAR_VERBOSE)) {
printf("%s\n", fs.path);
}
if(S_ISREG(fs.st.st_mode)) {
if(tar_cmd == TAR_LIST) {
if(lseek(tfd, fs.st.st_size, SEEK_CUR) == -1) {
call_error("lseek");
}
}
else {
if((fd = open(fs.path, O_WRONLY| O_CREAT, fs.st.st_mode)) == -1) { // problème lors de l'ouverture du fichier au chemin 'fs.path'
call_error(fs.path);
}
copie_fichier(tfd, fd, fs.st.st_size);
close(fd);
if(chown(fs.path, fs.st.st_uid, fs.st.st_gid) == -1) {
call_error(fs.path);
}
}
}
else if(S_ISLNK(fs.st.st_mode)) { // Si c'est un lien dur
	if(tar_cmd == TAR_LIST) {
		if(lseek(tfd, fs.st.st_size, SEEK_CUR) == -1) {
		call_error("lseek");
		}
	}
	else {
	file_buf = (char *)malloc(sizeof(char) * (fs.st.st_size + 1));
		if(!file_buf) {
		call_error("malloc");
		}
if(read(tfd, file_buf, fs.st.st_size) == -1) {
call_error("read");
}
if(symlink(file_buf, fs.path) == -1) {
call_error("symlink");
}
if(lchown(fs.path, fs.st.st_uid, fs.st.st_gid) == -1) {
call_error(fs.path);
}
free(file_buf);
}
}
else if(S_ISDIR(fs.st.st_mode)) {
if(tar_cmd != TAR_LIST) {
if(mkdir(fs.path, fs.st.st_mode) == -1) {
call_error(fs.path);
}
}
}
else if(S_ISCHR(fs.st.st_mode)) {
}
else if(S_ISBLK(fs.st.st_mode)) {
}
else if(S_ISFIFO(fs.st.st_mode)) {
}
else if(S_ISSOCK(fs.st.st_mode)) {
}
return 1;
}


int extraire_archive(const char *tar_path, int argc, char **argv)
{
int fd;
if(tar_path) {
if((fd = open(tar_path, O_RDONLY)) == -1) {
call_error(tar_path);
}
}
else fd = STDIN_FILENO; //Else read from STDIN
while(extract_next_file(fd, argc, argv));
if(fd > 2) close(fd);
}


int ajouter_fichier_archive(const char *file, int tfd, const char *tar_path)
{
int fd;
file_struct fs;
file_struct fs_old;
char * file_buf;
DIR * dir;
struct dirent * d_ent;
int write_file = 1;
if(lstat(file, &fs.st) == -1) {
call_error(file);
}
if(tar_cmd == TAR_UPDATE) {
if(tar_path) {
if((fd = open(tar_path, O_RDONLY)) == -1) {
call_error(tar_path);
}
}
else fd = STDIN_FILENO;
while(read(fd, &fs_old, sizeof(file_struct)) > 0) {
if(strcmp(file, fs_old.path) == 0) {
if(fs_old.st.st_mtime == fs.st.st_mtime && fs_old.st.st_ctime == fs.st.st_ctime) {
write_file = 0; /* Don't schedule to update this file, since it hasn't changed */
break;
}
}
if(!S_ISDIR(fs_old.st.st_mode)) {
if(lseek(fd, fs_old.st.st_size, SEEK_CUR) == -1) {
call_error("lseek");
}
}
}
if(fd > 2) close(fd);
}
if((tar_flag & TAR_VERBOSE) && write_file) {
printf("%s\n", file);
}
if(S_ISLNK(fs.st.st_mode) && (tar_flag & TAR_FOLLOW_SYMLINKS)) {
if(lstat(my_readlink(file), &fs.st) == -1) {
call_error(file);
}
}
strncpy(fs.path, virer_premier_slash(file), _POSIX_PATH_MAX);
if(write_file) {
if(write(tfd, &fs, sizeof(file_struct)) != sizeof(file_struct)) {	
call_error("write");
}
}
if(write_file && S_ISREG(fs.st.st_mode)) {
if((fd = open(file, O_RDONLY)) == -1) {
call_error(file);
}
copie_fichier(fd, tfd, fs.st.st_size);
close(fd);
}
else if(write_file && S_ISLNK(fs.st.st_mode)) {
if(write(tfd, my_readlink(file), fs.st.st_size) != fs.st.st_size) {
call_error("write");
}
}
else if(S_ISDIR(fs.st.st_mode)) {
file_buf = (char *)malloc(sizeof(char) * (_POSIX_PATH_MAX +1));
if((dir = opendir(file)) == NULL) {
call_error(file);
}
while((d_ent = readdir(dir)) != NULL) {
if(strcmp(d_ent->d_name, ".") && strcmp(d_ent->d_name, "..")) {
sprintf(file_buf, "%s/%s", virer_dernier_slash(file), d_ent->d_name);
ajouter_fichier_archive(file_buf, tfd, tar_path);
}
}
closedir(dir);
free(file_buf);
}
else if(S_ISCHR(fs.st.st_mode)) {
}
else if(S_ISBLK(fs.st.st_mode)) {
}
else if(S_ISFIFO(fs.st.st_mode)) {
}
else if(S_ISSOCK(fs.st.st_mode)) {
}
}


int creer_archive(const char *tar_path, int argc, char **argv)
{
int fd;
if(tar_path) {
if(tar_cmd == TAR_UPDATE || tar_cmd == TAR_APPEND) {
fd = open(tar_path, O_WRONLY| O_APPEND| O_CREAT, 0664);
} else {
fd = open(tar_path, O_WRONLY| O_TRUNC| O_CREAT, 0664);
}
if(fd == -1)
call_error(tar_path);
}
else fd = STDOUT_FILENO; // Else output to STDOUT
while(argc-- > 0) {
ajouter_fichier_archive(*(argv++), fd, tar_path);
}
if(fd > 2) close(fd);
}

void aide(const char *argv0, const char *msg)
{
if(msg) {
fprintf(stderr, "ERROR: %s\n\n", msg);
}
fprintf(stderr,
"USAGE: %s [OPTIONS] [FILE...]\n\n"
"Where OPTIONS is a string containing one of the following:\n\n"
"c Create the archive\n"
"x Extract the archive\n"
"t Test the archive\n"
"u Update the archive\n\n"
"Or additional flags:\n\n"
"f Input/Output from/to the specified file\n"
"v Verbose output\n"
,argv0);
exit(-1);
}


int Options(const char *opts, const char *argv0)
{
tar_flag = 0;
tar_cmd = 0;
while(*opts) {
switch (*opts) {
case 'x':
if(tar_cmd) aide(argv0, NULL);
tar_cmd = TAR_EXTRACT;
break;
case 'c':
if(tar_cmd) aide(argv0, NULL);
tar_cmd = TAR_CREATE;
break;
case 't':
if(tar_cmd) aide(argv0, NULL);
tar_cmd = TAR_LIST;
break;
case 'u':
if(tar_cmd) aide(argv0, NULL);
tar_cmd = TAR_UPDATE;
break;
case 'r':
if(tar_cmd) aide(argv0, NULL);
tar_cmd = TAR_APPEND;
break;
case 'v':
tar_flag |= TAR_VERBOSE;
break;
case 'f':
tar_flag |= TAR_FILE;
break;
case 'h':
tar_flag |= TAR_FOLLOW_SYMLINKS;
break;
default :
aide(argv0, "Unknown option");
}
opts++;
}
if(!tar_cmd) aide(argv0, NULL);
}


int main(int argc, char **argv)
{
int i;
int files_num = 0;
char ** files = NULL;
char *tarball_name = NULL;
if(argc < 2) {
aide(argv[0], NULL);
}
Options(argv[1], argv[0]);
argc -= 2;
argv += 2;
if(tar_flag & TAR_FILE) {
tarball_name = argv[0];
argc --;
argv ++;
}
switch (tar_cmd)
{
case TAR_CREATE:
case TAR_UPDATE:
case TAR_APPEND:
creer_archive(tarball_name, argc, argv);
break;
case TAR_EXTRACT:
case TAR_LIST:
extraire_archive(tarball_name, argc, argv);
break;
}
}


