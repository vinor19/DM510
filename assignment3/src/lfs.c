#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

int lfs_getattr( const char *, struct stat * );
int lfs_readdir( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_open( const char *, struct fuse_file_info * );
int lfs_read( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release(const char *, struct fuse_file_info *);
int lfs_mkdir( const char *, mode_t );
int lfs_rmdir( const char * );
int lfs_unlink( const char * );
int lfs_mknod( const char *, mode_t, dev_t );
int lfs_write( const char *, const char *, size_t, off_t, struct fuse_file_info * );
int lfs_truncate( const char *, off_t );
void lfs_destroy(struct fuse *f);

static struct fuse_operations lfs_oper = {
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod = lfs_mknod,
	.mkdir = lfs_mkdir,
	.unlink = lfs_unlink,
	.rmdir = lfs_rmdir,
	.truncate = lfs_truncate,
	.open	= lfs_open,
	.read	= lfs_read,
	.release = lfs_release,
	.write = lfs_write,
	.destroy = lfs_destroy,
	.rename = NULL,
	.utime = NULL
};

//Struct for both files and folders
struct lfs_file{
	//General things
	char *name; //Only name of the file/folder
	int file_type; //File = 1, Folder = 0
	char **path; //Path of the file/folder
	int depth;
	int maxSize;
	int entries; //Number of entries in directory;

	//Attributes
	uint64_t size; //Size of contents
	time_t access_time; //When was it last accessed
	time_t modification_time; //When was it last modified

	//Contents
	
	struct lfs_file **dir; //If Directory, this will be what is in the directory, if not it will be Null
	char *contents; //Contents of a file, if it is not a file, if will be Null
	
};

//Root directory
struct lfs_file *root;

//Counts how deep a path is
int count(const char *path){
    int count = 0;
    char *tmp = calloc(1,strlen(path));
    strcpy(tmp,path);
    char *pch;
    
    pch = strchr(tmp,'/');
    while(pch != NULL){
        pch = strchr(pch+1,'/');
        count++;
    }
    return count;
}

//Splits a path and puts it in array
char **pathSplit(char *path){
    char *tmp;
	char **array = calloc(count(path),sizeof(char*));
    tmp = strtok(path,"/");
	int i = 0;
    while(tmp!=NULL){
        array[i] = calloc(1,strlen(tmp));
		strcpy(array[i],tmp);
		tmp = strtok(NULL,"/");
		i++;
    }
    return array;
}

const char *pathRebuild(char**path, int length){
	if(length == 0)
		return "/";
	//printf("PathRebuild\n");
	char* result;
	int totallength = 0;
	//printf("PathRebuild first while\n");
	for(int i = 0; i<length; i++){
		//printf("%d\n",i);
		//printf("%s\n",path[i]);
		totallength = 1 + totallength + strlen(path[i]);
	}
	//printf("PathRebuild, totallength = %d\n", totallength);
	result = calloc (1,totallength*sizeof(char));
	//printf("PathRebuild, build allocated\n");
	for(int i = 0; i<length;i++){
		strcat(result,"/");
		strcat(result, path[i]);
	}
	printf("PathRebuild, string concatinated %s\n", result);
	return result;
}

struct lfs_file *findFile(const char* path){
	printf("findFile: (path=%s)\n", path);
	if(strcmp(path,"/") == 0){
		//printf("file was root\n");
		return root;
	}else{
		//printf("allocating and copying path\n");
		char *tmp = calloc(1,strlen(path) + 1);
		//printf("allocating and copying path\n");
		strcpy(tmp, path);
		//printf("Path copied, path is: %s\n", tmp);
		int depth = count(path);
		//printf("Depth found, depth is: %d\n", depth);
		char **splitpath = calloc(depth,sizeof(char*));
		splitpath = pathSplit(tmp);
		free(tmp);
		//printf("Path %s\n", splitpath[0]);
		//printf("Path has been split\n");
		struct lfs_file *foundfile = calloc(1,sizeof(struct lfs_file));
		foundfile = root;
		//printf("Foundfile is set to root\n");
		for(int i = 0; i < depth;i++){
			//printf("I: %d\n", i);
			for(int j = 0; j < foundfile->size && foundfile->dir != NULL; j++){
				//printf("J: %d\n",j);
				if (foundfile->dir[j] != NULL){
					//printf("Looking at: %s\n", foundfile->dir[j]->name);
					//printf("Compared too: %s\n", splitpath[i]);
					if (strcmp(foundfile->dir[j]->name, splitpath[i]) == 0){
						foundfile = foundfile->dir[j];
					}
				}
			}
			//printf("Currently in: %s\n", foundfile->name);
		}

		if (strcmp(splitpath[depth-1], foundfile->name) == 0 && depth == foundfile->depth){
			//printf("File was found on given path, file found was %s\n", foundfile->name);
			return foundfile;
		}else{

			//printf("File was not found on given path\n");
			return NULL;
		}
	}
}

//Makes a directory from path
int lfs_mkdir(const char *path, mode_t mode) {
	printf("mkdir: (path=%s)\n", path);
	(void)mode;
	struct lfs_file *file = calloc(1, sizeof(struct lfs_file));
	char *tmp = calloc(1,strlen(path) + 1);
	strcpy(tmp, path);
	file->depth = count(path);
	file->path = calloc(file->depth,sizeof(char*));
	file->path = pathSplit(tmp);
	file->name = calloc(strlen(file->path[file->depth-1]),sizeof(char));
	file->name = file->path[file->depth-1];
	//printf("Filename: %s\n", file->name);
	file->file_type = 0;
	file->size = 0;
	file->entries = 0;
	file->maxSize = 16;
	time(&file->access_time);
	time(&file->modification_time);
	file->dir = calloc(file->maxSize, sizeof(struct lfs_file *));
	file->contents = NULL;
	free(tmp);
	//printf("Assigned all of lfs_file\n");
	if(file->depth == 1){
		//printf("Creating in root directory\n");
		if(root->size >= root->maxSize){
			root->maxSize = root->maxSize * 2;
			struct lfs_file **newDir = calloc(root->maxSize, sizeof(struct lfs_file *));
			memcpy(newDir, root->dir,root->size * sizeof(struct lfs_file*));
			root->dir = newDir;
		}
		root->dir[root->size] = calloc(1,sizeof(struct lfs_file));
		root->dir[root->size] = file;
		root->size++;
		root->entries++;
		//printf("Foundfile Size: %ld\n", root->size);
		time(&root->access_time);
		time(&root->modification_time);
	}else if(file->depth > 1){
		//printf("Creating in %s directory\n", file->path[file->depth-2]);
		struct lfs_file *foundfile = calloc(1,sizeof(struct lfs_file));
		//printf("Finding file\n");
		foundfile = findFile(pathRebuild(file->path, file->depth-1));
		//printf("File found\n");
		if (foundfile == NULL)
			return -ENOENT;
		if(foundfile->size >= foundfile->maxSize){
			foundfile->maxSize = foundfile->maxSize * 2;
			struct lfs_file **newDir = calloc(foundfile->maxSize, sizeof(struct lfs_file *));
			memcpy(newDir, foundfile->dir,foundfile->size * sizeof(struct lfs_file*));
			foundfile->dir = newDir;
		}
		foundfile->dir[foundfile->size] = calloc(1,sizeof(struct lfs_file));
		foundfile->dir[foundfile->size] = file;
		foundfile->size++;
		foundfile->entries++;
		printf("Folder %s has been made in %s\n",file->name,file->path[file->depth-2]);
		time(&foundfile->access_time);
		time(&foundfile->modification_time);
	}
	
	return 0;
}

//Removes the director on that path
int lfs_rmdir(const char *path){
	printf("rmdir: (path=%s)\n", path);
	//printf("RMDIR STARTED\n");
	struct lfs_file *folderDelete = calloc(1,sizeof(struct lfs_file));
	folderDelete = findFile(path);
	//printf("Found Folder to delete %s\n", folderDelete->name);
	if(folderDelete->entries > 0)
		return -ENOTEMPTY;
	//printf("Folder is empty\n");
	struct lfs_file *folderDeleteParent = calloc(1,sizeof(struct lfs_file));
	folderDeleteParent = findFile(pathRebuild(folderDelete->path,folderDelete->depth-1));
	for(int i = 0; i<folderDeleteParent->maxSize;i++){
		//printf("First if statement\n");
		if(folderDeleteParent->dir[i] != NULL){
			//printf("Second if statement\n");
			if(strcmp(folderDeleteParent->dir[i]->name,folderDelete->name)==0){
				//printf("Found it\n");
				folderDeleteParent->dir[i] = NULL;
				//printf("Overwritten it\n");
			}
		}else{
			//printf("Looked at NULL\n");
		}
	}
	free(folderDelete->path);
	free(folderDelete->name);
	free(folderDelete->dir);
	free(folderDelete);
	//printf("Done freeing\n");
	folderDeleteParent->entries--;
	time(&folderDeleteParent->access_time);
	time(&folderDeleteParent->modification_time);
	return 0;
}

int lfs_mknod(const char *path, mode_t mode, dev_t device){
	printf("mknod: (path=%s)\n", path);
	struct lfs_file *file = calloc(1,sizeof(struct lfs_file));
	char *tmp = calloc(1,strlen(path) + 1);
	strcpy(tmp, path);
	file->depth = count(path);
	file->path = calloc(file->depth,sizeof(char*));
	file->path = pathSplit(tmp);
	file->name = calloc(strlen(file->path[file->depth-1]),sizeof(char));
	file->name = file->path[file->depth-1];
	//printf("Filename: %s\n", file->name);
	file->file_type = 1;
	file->size = 0;
	file->entries = -1;
	file->maxSize = 8192;
	time(&file->access_time);
	time(&file->modification_time);
	file->dir = NULL;
	file->contents = calloc(1,file->maxSize*sizeof(char));
	free(tmp);
	//printf("Assigned all of lfs_file\n");
	if(file->depth == 1){
		//printf("Creating in root directory\n");
		root->dir[root->size] = calloc(1,sizeof(struct lfs_file));
		root->dir[root->size] = file;
		root->size++;
		root->entries++;
		//printf("Foundfile Size: %ld\n", root->size);
		time(&root->access_time);
		time(&root->modification_time);
		//printf("Node has been assigned in root\n");
	}else if(file->depth > 1){
		//printf("Creating in %s directory\n", file->path[file->depth-2]);
		struct lfs_file *foundfile = calloc(1,sizeof(struct lfs_file));
		//printf("Finding file\n");
		foundfile = findFile(pathRebuild(file->path, file->depth-1));
		//printf("File found\n");
		if (foundfile == NULL)
			return -ENOENT;
		foundfile->dir[foundfile->size] = calloc(1,sizeof(struct lfs_file));
		foundfile->dir[foundfile->size] = file;
		foundfile->size++;
		foundfile->entries++;
		//printf("Foundfile Size: %ld\n", foundfile->size);
		time(&foundfile->access_time);
		time(&foundfile->modification_time);
		//printf("Node has been assigned in %s\n", foundfile->name);
	}
	
	return 0;
}

int lfs_unlink(const char *path){
	printf("unlink: (path=%s)\n", path);
	//printf("UNLINK STARTED\n");
	struct lfs_file *fileDelete = calloc(1,sizeof(struct lfs_file));
	fileDelete = findFile(path);
	//printf("Found file to delete\n");
	if(fileDelete == NULL)
		return -ENOENT;

	struct lfs_file *fileDeleteParent = calloc(1,sizeof(struct lfs_file));
	fileDeleteParent = findFile(pathRebuild(fileDelete->path,fileDelete->depth-1));
	if(fileDeleteParent == NULL)
		return -ENOENT;
	//printf("Removing from table: %s\n", fileDelete->name);
	for(int i = 0; i<fileDeleteParent->size;i++){
		//printf("First if statement\n");
		if(fileDeleteParent->dir[i] != NULL){
			//printf("Second if statement\n");
			if(strcmp(fileDeleteParent->dir[i]->name,fileDelete->name)==0){
				//printf("Found it\n");
				fileDeleteParent->dir[i] = NULL;
				//printf("Overwritten it\n");
			}
		}
	}
	free(fileDelete->name);
	free(fileDelete->path);
	free(fileDelete->contents);
	free(fileDelete);
	//printf("Done freeing\n");
	fileDeleteParent->entries--;
	time(&fileDeleteParent->access_time);
	time(&fileDeleteParent->modification_time);
	return 0;
}

int lfs_getattr( const char *path, struct stat *stbuf ) {
	printf("%ld\n",sizeof(struct lfs_file));
	printf("Get attribute started\n");
	int res = 0;
	struct lfs_file *file = calloc(1,sizeof(struct lfs_file));
	printf("getattr: (path=%s)\n", path);
	printf("Finding file\n");
	file = findFile(path);
	printf("Found something\n");


	memset(stbuf, 0, sizeof(struct stat));
	printf("stbuf set to 0\n");
	if( strcmp( path, "/" ) == 0 ) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}else if(file != NULL){
			if(file->file_type == 0){
				stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 2;
			}else if (file->file_type == 1){
				stbuf->st_nlink = 1;
				stbuf->st_mode = S_IFREG | 0777;
			}else{
				res = -ENOENT;
			}
			stbuf->st_size = file->size;
			stbuf->st_atime = file->access_time;
			stbuf->st_mtime = file->modification_time;
		} else
			res = -ENOENT;
	printf("What is res: %d\n", res);
	return res;
}

int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	printf("readdir: (path=%s)\n", path);
	(void) offset;
	(void) fi;
	

	printf("Filler . and ..\n");
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	struct lfs_file *foundFile = calloc(1,sizeof(struct lfs_file));
	foundFile = findFile(path);
	printf("Foundfile %s\n",foundFile->name);
	if(foundFile != NULL){
		//printf("FolderSize: %ld\n",foundFile->size);
		//printf("FolderName: %s\n",foundFile->name);
		for(int i = 0; i<foundFile->size;i++){
			//printf("Starting to look through files\n");
			if(foundFile->dir[i] != NULL){
				//printf("File was not Null\n");
				//printf("Filename: %s\n", (&foundFile->dir[i])->name);
				filler(buf, foundFile->dir[i]->name,NULL,0);
			}else{
				//printf("Spot was empty\n");
			}
		}
	}else{
		//printf("File not found\n");
	}
	return 0;
}


//Permission
int lfs_open( const char *path, struct fuse_file_info *fi ) {
    printf("open: (path=%s)\n", path);
	struct lfs_file *file = calloc(1,sizeof(struct lfs_file));
	if((file = findFile(path)) == NULL)
		return -ENOENT;
	fi->fh = (uint64_t) file;
	return 0;
}

//Truncates
int lfs_truncate(const char *path, off_t offset){
	printf("truncate: (path=%s)\n", path);
	struct lfs_file *file = calloc(1,sizeof(struct lfs_file));
	file = findFile(path);

	if(file == NULL)
		return -ENOENT;
	file->size = offset;
	time(&file->access_time);
	time(&file->modification_time);
	return 0;
}

int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
    printf("read: (path=%s)\n", path);
	struct lfs_file *file = calloc(1,sizeof(struct lfs_file));
	file = findFile(path);
	//file = (struct lfs_file*) fi->fh;
	if(file == NULL)
		return -ENOENT;
	
	int read = size;
	if(offset+size > file->size)
		read = file->size - offset;

	memcpy( buf, file->contents+offset, read );
	time(&file->access_time);
	return read;
}

int lfs_write(const char *path, const char *content, size_t size, off_t offset, struct fuse_file_info *fi){
	printf("read: (path=%s)\n", path);
	struct lfs_file *file = calloc(1,sizeof(struct lfs_file));
	file = findFile(path);
	//file = (struct lfs_file*) fi->fh;
	if(file == NULL)
		return -ENOENT;
	
	int sizeAfterWrite = offset + size;

	if(sizeAfterWrite > file->maxSize){
		file->maxSize = sizeAfterWrite*2;
		char *newContents = calloc (1,file->maxSize);
		memcpy(newContents,file->contents,file->size);
		file->contents = newContents;
	}
	memcpy( file->contents + offset, content, size );
	if(sizeAfterWrite > file->size)
		file->size = sizeAfterWrite;
	time(&file->access_time);
	time(&file->modification_time);
	return size;
}

int lfs_release(const char *path, struct fuse_file_info *fi) {
	printf("release: (path=%s)\n", path);
	return 0;
}

//Write file to disk
void lfs_writeFileToDisk(struct lfs_file *file, FILE *disk){
	printf("Writing %s to disk\n", file->name);
	fwrite(&file->file_type, sizeof(int), 1, disk); //File type
	const char *path = pathRebuild(file->path,file->depth);
	printf("%s\n", path);
	size_t length = strlen(path);
	fwrite(&length,sizeof(size_t),1,disk); //Length of path
	fwrite(path, strlen(path), 1, disk); //The path itself
	printf("%s\n", path);
	//Meta info
	fwrite(&file->access_time, sizeof(time_t),1,disk);
	fwrite(&file->modification_time, sizeof(time_t),1,disk);
	fwrite(&file->size, sizeof(uint64_t), 1, disk);
	//Contents
	fwrite(file->contents, file->size, 1, disk);
	lfs_unlink(path);
}

//Write folder to disk
void lfs_writeFolderToDisk(struct lfs_file *folder, FILE *disk){
	printf("Writing %s to disk\n", folder->name);
	fwrite(&folder->file_type, sizeof(int), 1, disk);//File type
	const char *path = pathRebuild(folder->path,folder->depth);
	printf("%s\n", path);
	size_t length = strlen(path);
	fwrite(&length,sizeof(size_t),1,disk);//Path length
	fwrite(path, strlen(path), 1, disk);//Path
	printf("%s\n", path);
	//Metainfo
	fwrite(&folder->access_time, sizeof(time_t),1,disk);
	fwrite(&folder->modification_time, sizeof(time_t),1,disk);
	//Writes all files in folder
	for(int i = 0; i < folder->size; i++)
		if(folder->dir[i] != NULL){
			if(folder->dir[i]->file_type == 0)
				lfs_writeFolderToDisk(folder->dir[i], disk);
			else
				lfs_writeFileToDisk(folder->dir[i], disk);
		}
	lfs_rmdir(path);
}

void lfs_destroy(struct fuse *f){
	printf("Destroy started\n");
	FILE *disk = fopen("/tmp/disk.img","wb");
	for(int i = 0; i < root->size; i++)
		if(root->dir[i] != NULL){
			if(root->dir[i]->file_type == 0)
				lfs_writeFolderToDisk(root->dir[i], disk);
			else
				lfs_writeFileToDisk(root->dir[i], disk);
		}
	free(root->path);
	free(root->dir);
	free(root);
	fclose(disk);
}

int main( int argc, char *argv[] ) {
	FILE *disk = fopen("/tmp/disk.img","rb");
	printf("%ld\n",sizeof(struct lfs_file));
	root = calloc(1,sizeof(struct lfs_file));
	root->name = "/";
	root->path = calloc(1,sizeof(char*));
	root->path[0] = "/";
	root->depth = 0;
	root->file_type = 0;
	root->size = 0;
	root->maxSize = 16;
	time(&root->access_time);
	time(&root->modification_time);
	root->dir = calloc(16, sizeof(struct lfs_file *));
	root->contents = NULL;
	uint64_t size = 0;
	size_t pathlength = 0;
	char *path;
	int filetype = 0;
	printf("Starting to read the file\n");
	if(disk != NULL){
		while(fread(&filetype, sizeof(int), 1, disk)>0){
			printf("Read filetype, it is %d\n", filetype);
			fread(&pathlength, sizeof(size_t), 1, disk);
			printf("Read path length, it is %ld\n", pathlength);
			path = calloc(1,pathlength);
			fread(path, pathlength, 1, disk);
			printf("Read path, it is %s\n", path);
			if(filetype == 0){
				lfs_mkdir(path, NULL);
				struct lfs_file *folder = calloc(1,sizeof(struct lfs_file));
				folder = findFile(path);
				fread(&folder->access_time, sizeof(time_t), 1, disk);
				fread(&folder->modification_time, sizeof(time_t), 1, disk);
			}
			else if (filetype == 1){
				lfs_mknod(path,NULL,NULL);
				struct lfs_file *file = calloc(1,sizeof(struct lfs_file));
				file = findFile(path);
				fread(&file->access_time, sizeof(time_t), 1, disk);
				fread(&file->modification_time, sizeof(time_t), 1, disk);
				fread(&file->size,sizeof(uint64_t),1,disk);
				if(file->size>8192){ //8192 is standard size for files, if it has to read more than that, it needs to update the buffer size
					file->maxSize = file->size * 2;
					char *newContents = calloc (1,file->maxSize);
					memcpy(newContents,file->contents,file->size);
					file->contents = newContents;
				}
				fread(file->contents,file->size,1,disk);
			}
			else {
				return -69;
			}
			free(path);
		}
		fclose(disk);
	}

	fuse_main( argc, argv, &lfs_oper );
	return 0;
}
