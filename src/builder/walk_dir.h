#include <dirent.h>
#include <shared/raii.h>
#include <sys/stat.h>



namespace WalkDir {

using ondra_shared::RAII;

enum WalkEvent {
	///standard directory entry
	file_entry,
	///entering to subdirectory
	/** note if the callback function returns false, the walking is not
	 * canceled, it just prevents to walk into subdirectory
	 */
	directory_enter,
	///leaving the subdirectory
	/** in contrast to directory_enter, by returning false now causes
	 * canceling the walk complete
	 */
	directory_leave,
};

template<typename Fn>
auto walk_directory_callcb(Fn &&cb, const std::string &path, WalkEvent) -> decltype(cb(path) ){
	return cb(path);
}
template<typename Fn>
auto walk_directory_callcb(Fn &&cb, const std::string &path, WalkEvent rt) -> decltype(cb(path,rt ) ){
	return cb(path,rt);
}

static inline bool isDots(const char *name) {
	return name[0] == '.' && ((name[1] == '.' && name[2] == 0)||name[1] == 0);
}

inline void myclosedir(DIR *dir) {closedir(dir);}

///Walks directory from given path and calls callback function for each entry
/**
 *
 * @param path starting path
 * @param recursive process directory tree recursively. If this set to true,
 *  it stills reports entering and leaving directories, but all of them are appear empty.
 * @param cb callback function. It can have one or two arguments. With one argument
 * it accepts only full pathname (pathname concatelated with path argument). If
 * the function has two arguments, then second argument contains WalkEvent value.
 * The function must return boolen, where true is meant to continue walking while
 * false causes to stop walking. Also note, that directory_leave events are
 * still called when the walker is leaving structure during finishing interrupted
 * walking.
 * @retval true finished
 * @retval false canceled
 */
template<typename Fn>
bool walk_directory(std::string path, bool recursive, Fn &&cb) {



	typedef RAII<DIR *, decltype(&myclosedir), &myclosedir> Dir;

	DIR *dirptr = opendir(path.c_str());
	if (dirptr == nullptr) return false;

	Dir dir(dirptr);


	const dirent *entry;

	auto pathlen = path.length();

	while ((entry = readdir(dir)) != nullptr) {
		if (!isDots(entry->d_name)) {
			path.resize(pathlen);
			path.push_back('/');
			path.append(entry->d_name);
			bool isdir;
			if (entry->d_type == DT_UNKNOWN) {
				struct stat entrystat;
				lstat(path.c_str(),&entrystat );
				isdir = S_ISDIR(entrystat.st_mode);
			} else {
				isdir = entry->d_type == DT_DIR;
			}

			if (isdir) {
				if (walk_directory_callcb(std::forward<Fn>(cb),path,directory_enter)) {
					bool x = !recursive || walk_directory(path, recursive, std::forward<Fn>(cb));
					bool y = walk_directory_callcb(std::forward<Fn>(cb),path,directory_leave);
					if (!(x && y)) return false;
				}
			} else {
				if (!walk_directory_callcb(std::forward<Fn>(cb),path,file_entry)) {
					return false;
				}
			}
		}
	}
	return true;
}


};
