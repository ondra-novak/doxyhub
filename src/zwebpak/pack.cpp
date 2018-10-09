/*
 * pack.cpp
 *
 *  Created on: 4. 10. 2018
 *      Author: ondra
 */

#include <string>
#include <vector>
#include <iostream>
#include "../builder/walk_dir.h"
#include "zwebpak.h"


int main(int argc, char *argv[]) {


	std::vector<std::string> files;


	if (argc != 3) {
		std::cerr << "Need two arguments: <name_of_archive> <source_root_directory>" << std::endl;
		return 2;
	}

	std::string name = argv[1];
	std::string root_dir = argv[2];
	if (root_dir[root_dir.length()] == '/') root_dir.pop_back();
	auto strip =root_dir.length()+1;

	WalkDir::walk_directory(root_dir, true, [&](const std::string &path, WalkDir::WalkEvent ev){
		if (ev == WalkDir::file_entry) {
			files.push_back(path.substr(strip));
		}
		return true;
	});

	root_dir.push_back('/');

	if (zwebpak::packFiles(ondra_shared::StringView<std::string>(files.data(), files.size()), root_dir, name,"testtest", 256*1024)) {
		std::cerr << "Success" << std::endl;
		return 0;
	} else {
		std::cerr << "Failure" << std::endl;
		return 1;
	}




}
